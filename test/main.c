#define USE_WOLFSSL 1
#ifdef USE_WOLFSSL
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/sniffer.h>
#else
#include <openssl/ssl.h>
#endif

#include <event2/event.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#ifndef EVENT__HAVE_OPENSSL
#define EVENT__HAVE_OPENSSL 1
#endif
#include <event2/bufferevent_ssl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TEST_METHOD EVHTTP_REQ_GET
#define TEST_NAMESERVER "192.168.251.1:53"
#define TEST_URI "https://192.168.251.50:9443/"
typedef struct
{
    struct evhttp_uri *uri;
    struct evhttp_request *request;
    struct bufferevent *bev;
    struct evhttp_connection *conn;
    SSL_CTX *sctx;
    SSL *ssl;
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
    if (p->bev)
    {
        bufferevent_free(p->bev);
    }
    if (p->ssl)
    {
        SSL_free(p->ssl);
    }
    if (p->sctx)
    {
        SSL_CTX_free(p->sctx);
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
        request_free(arg);
        puts("error req NULL");
        return;
    }
    puts("on_request_cb");
    int code = evhttp_request_get_response_code(req);
    if (code)
    {
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
    request_free(arg);
}
void on_error_cb(enum evhttp_request_error err, void *arg)
{
    switch (err)
    {
    case EVREQ_HTTP_TIMEOUT:
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
int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    // https://www.openssl.org/docs/manmaster/man3/SSL_CTX_set_verify.html
    char buf[256];
    X509 *err_cert;
    int err, depth;
    SSL *ssl;

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);
    ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

    printf("----------- depth=%d ok=%d %s\n", depth, preverify_ok, buf);
    return 1;
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
    const char *scheme = evhttp_uri_get_scheme(req->uri);
    printf("scheme: %s\n", scheme);
    const char *host = evhttp_uri_get_host(req->uri);
    printf("host: %s\n", host);
    int port = evhttp_uri_get_port(req->uri);
    if (!strcmp(scheme, "http"))
    {
        if (port < 1)
        {
            port = 80;
        }
    }
    else if (!strcmp(scheme, "https"))
    {
        if (port < 1)
        {
            port = 443;
        }
    }
    else
    {
        printf("scheme not supported: %s\n", scheme);
        goto END;
    }
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

    if (TEST_METHOD == EVHTTP_REQ_POST ||
        TEST_METHOD == EVHTTP_REQ_PUT ||
        TEST_METHOD == EVHTTP_REQ_PATCH)
    {
        struct evbuffer *buf = evhttp_request_get_output_buffer(req->request);
        const char *body = "{\"id\":1,\"name\":\"kate\"}";
        if (evbuffer_add(buf, body, strlen(body)))
        {
            puts("evbuffer_add error");
            goto END;
        }
    }
    // 創建 socket 連接
    if (strcmp(scheme, "http"))
    {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        // 創建 ssl 上下文
        req->sctx = SSL_CTX_new(TLS_client_method());
        if (!req->sctx)
        {
            puts("SSL_CTX_new error");
            goto END;
        }

        SSL_CTX_set_timeout(req->sctx, 3000);
        // 設置如何驗證服務器證書
        SSL_CTX_set_verify(req->sctx, SSL_VERIFY_PEER, verify_callback);
        SSL_CTX_set_default_verify_paths(req->sctx);

        // 創建 ssl 層
        req->ssl = SSL_new(req->sctx);
        if (!req->ssl)
        {
            puts("SSL_new error");
            goto END;
        }
        // set sni
        if (SSL_set_tlsext_host_name(req->ssl, host) != SSL_SUCCESS)
        {
            puts("SSL_set_tlsext_host_name error");
            goto END;
        }

        req->bev = bufferevent_openssl_socket_new(eb, -1, req->ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        req->ssl = NULL; // 設置了 BEV_OPT_CLOSE_ON_FREE 無論成功與否 都會自動 釋放 ssl，這裏置空避免兩次釋放
        if (!req->bev)
        {
            puts("bufferevent_openssl_socket_new error");
            goto END;
        }
    }
    else
    {
        req->bev = bufferevent_socket_new(eb, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (!req->bev)
        {
            puts("bufferevent_socket_new error");
            goto END;
        }
    }
    // 創建 http 連接
    req->conn = evhttp_connection_base_bufferevent_new(eb, esb, req->bev, host, port);
    if (!req->conn)
    {
        puts("connect error");
        goto END;
    }
    req->bev = NULL; // conn 會自動釋放，這裏置空避免被 釋放兩次

    evhttp_connection_set_closecb(req->conn, on_connection_close, NULL);
    evhttp_connection_set_timeout(req->conn, 10);
    // 發送請求
    int sz_path = path ? strlen(path) : 0;
    int sz_query = query ? strlen(query) : 0;
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
        ret = evhttp_make_request(req->conn, req->request, TEST_METHOD, uri);
        free(uri);
    }
    else
    {
        ret = evhttp_make_request(req->conn, req->request, TEST_METHOD, path);
    }
    if (ret)
    {
        puts("evhttp_make_request error");
        goto END;
    }
    req->request = NULL; // 回調後 evhhtp 會自動釋放資源，避免多次釋放
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
#ifdef TEST_NAMESERVER
    esb = evdns_base_new(eb, EVDNS_BASE_DISABLE_WHEN_INACTIVE);
    if (!esb)
    {
        puts("evdns_base_new error");
        goto END;
    }
    if (evdns_base_nameserver_ip_add(esb, TEST_NAMESERVER))
    {
        puts("evdns_base_nameserver_ip_add error");
        goto END;
    }
#else
    esb = evdns_base_new(eb, EVDNS_BASE_INITIALIZE_NAMESERVERS | EVDNS_BASE_DISABLE_WHEN_INACTIVE);
    if (!esb)
    {
        puts("evdns_base_new error");
        goto END;
    }
#endif

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
    puts("end");
    return ret;
}