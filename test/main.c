#include <event2/event.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TEST_URI "http://localhost:9000/abc?id=1&level=2#kate"
typedef struct
{
    struct evhttp_uri *uri;
    struct evhttp_request *request;
    struct evhttp_connection *conn;
} request_t;

void request_free(request_t *p)
{
    if (p->uri)
    {
        evhttp_uri_free(p->uri);
    }
    if (p->request)
    {
        evhttp_request_free(p->request);
    }
    if (p->conn)
    {
        evhttp_connection_free(p->conn);
    }
    free(p);
}
void on_connection_close(struct evhttp_connection *conn, void *arg)
{
    puts("on_connection_close");
}
void on_request_cb(struct evhttp_request *req, void *arg)
{
    if (!req)
    {
        puts("error req NULL");
        return;
    }
    puts("on_request_cb");
    int code = evhttp_request_get_response_code(req);
    const char *line = evhttp_request_get_response_code_line(req);
    printf("code: %d %s\n", code, line);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    size_t n = evbuffer_get_length(buf);
    if (n > 0)
    {
        char *body = malloc(n + 1);
        if (body)
        {
            n = evbuffer_remove(buf, body, n);
            body[n] = 0;
            puts(body);
        }
        else
        {
            printf("malloc body(%zu) error\n", n);
        }
    }
}
void on_error_cb(enum evhttp_request_error err, void *arg)
{
    switch (EVREQ_HTTP_TIMEOUT)
    {
    case EVREQ_HTTP_TIMEOUT:
        puts("Timeout reached.");
        break;
    case EVREQ_HTTP_EOF:
        puts("EOF reached.");
        break;
    case EVREQ_HTTP_INVALID_HEADER:
        puts("Error while reading header, or invalid header.");
        break;
    case EVREQ_HTTP_BUFFER_ERROR:
        puts("Error encountered while reading or writing.");
        break;
    case EVREQ_HTTP_REQUEST_CANCEL:
        puts("The evhttp_cancel_request() called on this request.");
        break;
    case EVREQ_HTTP_DATA_TOO_LONG:
        puts("Body is greater then evhttp_connection_set_max_body_size()");
        break;
    default:
        puts("unknow error");
        break;
    }
}
int http_example(struct event_base *eb, struct evdns_base *esb)
{
    int ret = -1;
    request_t *req = NULL;
    // 動態申請內存，存儲請求資源
    req = malloc(sizeof(request_t));
    if (!req)
    {
        puts("malloc request_t error");
        goto END;
    }
    memset(req, 0, sizeof(request_t));
    // 解析請求 uri
    req->uri = evhttp_uri_parse(TEST_URI);
    if (!req->uri)
    {
        printf("not a valid uri: %s\n", TEST_URI);
        goto END;
    }
    puts("new request");
    printf("scheme: %s\n", evhttp_uri_get_scheme(req->uri));
    const char *host = evhttp_uri_get_host(req->uri);
    printf("host: %s\n", host);
    int port = evhttp_uri_get_port(req->uri);
    printf("port: %d\n", port);
    printf("userinfo: %s\n", evhttp_uri_get_userinfo(req->uri));
    const char *path = evhttp_uri_get_path(req->uri);
    printf("path: %s\n", path);
    const char *query = evhttp_uri_get_query(req->uri);
    printf("query: %s\n", query);
    printf("fragment: %s\n", evhttp_uri_get_fragment(req->uri));

    // 創建請求
    req->request = evhttp_request_new(on_request_cb, req);
    if (!req->request)
    {
        puts("evhttp_request_new error");
        goto END;
    }
    // 設置錯誤處理
    evhttp_request_set_error_cb(req->request, on_error_cb);
    // 設置 headers
    struct evkeyvalq *headers = evhttp_request_get_output_headers(req->request);
    evhttp_add_header(headers, "Host", host); // Host 是必須設置的 其它可選
    evhttp_add_header(headers, "User-Agent", "iotjs");

    struct evbuffer *buf = evhttp_request_get_output_buffer(req->request);
    const char *body = "{\"id\":1,\"name\":\"kate\"}";
    if (evbuffer_add(buf, body, strlen(body)))
    {
        puts("evbuffer_add error");
        goto END;
    }

    // 創建連接
    req->conn = evhttp_connection_base_bufferevent_new(eb, esb, NULL, host, port);
    if (!req->conn)
    {
        puts("connect error");
        goto END;
    }
    evhttp_connection_set_closecb(req->conn, on_connection_close, NULL);
    evhttp_connection_set_timeout(req->conn, 10);

    // 發送請求
    int sz_path = strlen(path);
    int sz_query = strlen(query);
    if (!sz_path)
    {
        path = "/";
        sz_path = 1;
    }
    if (sz_query)
    {
        char *uri = malloc(sz_path + sz_query + 2);
        if (!uri)
        {
            puts("malloc uri error");
            goto END;
        }
        memcpy(uri, path, sz_path);
        uri[sz_path] = '?';
        memcpy(uri + sz_path + 1, query, sz_query);
        uri[sz_path + 1 + sz_query] = 0;
        ret = evhttp_make_request(req->conn, req->request, EVHTTP_REQ_PUT, uri);
        free(uri);
    }
    else
    {
        ret = evhttp_make_request(req->conn, req->request, EVHTTP_REQ_PUT, path);
    }
    if (ret)
    {
        puts("evhttp_make_request error");
        goto END;
    }
    ret = 0;
END:
    if (!ret)
    {
        return 0;
    }
    if (req)
    {
        request_free(req);
    }
    return ret;
}
int main(int argc, char *argv[])
{
    int ret = -1;
    struct event_base *eb = NULL;
    struct evdns_base *esb = NULL;
    eb = event_base_new();
    if (!eb)
    {
        puts("event_base_new error");
        goto END;
    }
    esb = evdns_base_new(eb, EVDNS_BASE_INITIALIZE_NAMESERVERS | EVDNS_BASE_DISABLE_WHEN_INACTIVE);
    if (!esb)
    {
        puts("evdns_base_new error");
        goto END;
    }
    if (http_example(eb, esb))
    {
        goto END;
    }
    ret = event_base_dispatch(eb);
    if (ret < 0)
    {
        puts("event_base_dispatch error");
        goto END;
    }
    esb = NULL;
    ret = 0;
END:
    if (esb)
    {
        evdns_base_free(esb, 0);
    }
    if (eb)
    {
        event_base_free(eb);
    }
    return ret;
}