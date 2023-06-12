#include <iotjs/core/configure.h>
#include <iotjs/core/memory.h>
#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <iotjs/core/async.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/sniffer.h>

#ifndef EVENT__HAVE_OPENSSL
#define EVENT__HAVE_OPENSSL 1
#endif
#include <event2/bufferevent_ssl.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/event.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <iotjs/modules/js/net_http.h>
#include <string.h>
#include <stdlib.h>

#define _VM_HTTP_WEBSOCKET_STEP_CONNECT 0
#define _VM_HTTP_WEBSOCKET_STEP_HANDSHAKE 1
#define _VM_HTTP_WEBSOCKET_STEP_READY 2

#define _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC -1
#define _VM_HTTP_WEBSOCKET_CONNECT_ERROR_TIMEOUT 1
#define _VM_HTTP_WEBSOCKET_CONNECT_ERROR_SOCKET 2
#define _VM_HTTP_WEBSOCKET_CONNECT_ERROR_ENABLE_READ 3
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_EOF 10
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_TIMEOUT 11
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_ERROR 12
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_EOF 13
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_TIMEOUT 14
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_ERROR 15
#define _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_PROTOCOL_ERROR 20
#define _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_UPGRADE 0x1
#define _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_CONNECTION 0x2
#define _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_SEC 0x4

static void http_uri_free(void *arg)
{
#ifdef VM_TRACE_FINALIZER
    puts(" --------- http_uri_free");
#endif
    if (arg)
    {
        evhttp_uri_free(arg);
    }
}
static duk_ret_t _native_uri_parse(duk_context *ctx)
{
    const char *url = duk_require_string(ctx, 0);
    finalizer_t *finalizer = vm_create_finalizer(ctx);
    struct evhttp_uri *uri = evhttp_uri_parse(url);
    duk_remove(ctx, -2);
    if (!uri)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "parse url error");
        duk_throw(ctx);
    }
    finalizer->free = http_uri_free;
    finalizer->p = uri;

    duk_push_object(ctx);
    const char *scheme = evhttp_uri_get_scheme(uri);
    const int len_scheme = scheme ? strlen(scheme) : 0;
    if (len_scheme)
    {
        duk_push_lstring(ctx, scheme, len_scheme);
        duk_put_prop_lstring(ctx, -2, "scheme", 6);
    }
    else
    {
        duk_push_lstring(ctx, "", 0);
        duk_put_prop_lstring(ctx, -2, "scheme", 6);
    }
    const char *host = evhttp_uri_get_host(uri);
    const int len_host = host ? strlen(host) : 0;
    if (len_host)
    {
        duk_push_lstring(ctx, host, len_host);
        duk_put_prop_lstring(ctx, -2, "host", 4);
    }
    else
    {
        duk_push_lstring(ctx, "", 0);
        duk_put_prop_lstring(ctx, -2, "host", 4);
    }
    int port = evhttp_uri_get_port(uri);
    if (port > 0)
    {
        duk_push_int(ctx, port);
        duk_put_prop_lstring(ctx, -2, "port", 4);
    }
    const char *path = evhttp_uri_get_path(uri);
    const int len_path = path ? strlen(path) : 0;
    if (len_path)
    {
        duk_push_lstring(ctx, path, len_path);
        duk_put_prop_lstring(ctx, -2, "path", 4);
    }
    else
    {
        duk_push_lstring(ctx, "/", 1);
        duk_put_prop_lstring(ctx, -2, "path", 4);
    }
    const char *query = evhttp_uri_get_query(uri);
    const int len_query = query ? strlen(query) : 0;
    if (len_query)
    {
        duk_push_lstring(ctx, query, len_query);
        duk_put_prop_lstring(ctx, -2, "query", 5);
    }

    evhttp_uri_free(finalizer->p);
    finalizer->free = NULL;
    return 1;
}
typedef struct
{
    vm_context_t *vm;
    struct evhttp_connection *conn;
    SSL_CTX *sctx;
    duk_uint8_t closed;
} http_conn_t;
static void http_conn_on_close(struct evhttp_connection *conn, void *arg)
{
    http_conn_t *p = arg;
    p->closed = 1;
}
static void http_conn_free(void *arg)
{
#ifdef VM_TRACE_FINALIZER
    puts(" --------- http_conn_free");
#endif
    if (!arg)
    {
        return;
    }
    http_conn_t *p = arg;
    if (p->conn)
    {
        evhttp_connection_free(p->conn);
        p->conn = NULL;
    }
    if (p->sctx)
    {
        SSL_CTX_free(p->sctx);
        p->sctx = NULL;
    }
    vm_context_t *vm = p->vm;
    if (vm)
    {
        vm_free_dns(vm);
        p->vm = NULL;
    }
}

static int tls_verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    return 1;
}
static duk_ret_t _native_connect(duk_context *ctx)
{
    duk_bool_t tls = duk_require_boolean(ctx, 0);
    const char *host = duk_require_string(ctx, 1);
    duk_uint_t port = duk_require_uint(ctx, 2);
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(http_conn_t));
    finalizer->free = http_conn_free;
    http_conn_t *p = finalizer->p;

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    p->vm = vm;
    vm->dns++;
    struct bufferevent *bev;
    if (tls)
    {
        p->sctx = SSL_CTX_new(TLS_client_method());
        if (!p->sctx)
        {
            duk_error(ctx, DUK_ERR_ERROR, "SSL_CTX_new error");
        }
        SSL_CTX_set_timeout(p->sctx, 3000);
        SSL_CTX_set_verify(p->sctx, SSL_VERIFY_PEER, tls_verify_callback);
        SSL_CTX_set_default_verify_paths(p->sctx);
        SSL *ssl = SSL_new(p->sctx);
        if (!ssl)
        {
            duk_error(ctx, DUK_ERR_ERROR, "SSL_new error");
        }
        // set sni
        if (SSL_set_tlsext_host_name(ssl, host) != SSL_SUCCESS)
        {
            SSL_free(ssl);
            duk_error(ctx, DUK_ERR_ERROR, "SSL_set_tlsext_host_name error");
        }
        bev = bufferevent_openssl_socket_new(vm->eb, -1, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (!bev)
        {
            duk_error(ctx, DUK_ERR_ERROR, "bufferevent_openssl_socket_new error");
        }
    }
    else
    {
        bev = bufferevent_socket_new(vm->eb, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (!bev)
        {
            duk_error(ctx, DUK_ERR_ERROR, "bufferevent_socket_new error");
        }
    }

    p->conn = evhttp_connection_base_bufferevent_new(vm->eb, vm->esb, bev, host, port);
    if (!p->conn)
    {
        bufferevent_free(bev);
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_connection_base_bufferevent_new error");
    }
    evhttp_connection_set_closecb(p->conn, http_conn_on_close, p);
    return 1;
}
static duk_ret_t _native_free_connect(duk_context *ctx)
{
    vm_finalizer_free(ctx, 0, http_conn_free);
    return 0;
}

static duk_ret_t _native_is_close_conn(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_conn_free);
    http_conn_t *p = finalizer->p;
    duk_bool_t closed = p->closed;
    duk_pop(ctx);
    if (closed)
    {
        duk_push_true(ctx);
    }
    else
    {
        duk_push_false(ctx);
    }
    return 1;
}
typedef struct
{
    vm_context_t *vm;
    struct evhttp_request *req;
    enum evhttp_request_error err;
    duk_bool_t hasErr;
} http_request_t;
static void http_request_free(void *arg)
{
#ifdef VM_TRACE_FINALIZER
    puts(" --------- http_request_free");
#endif
    if (!arg)
    {
        return;
    }
    http_request_t *p = arg;
    if (p->req)
    {
        evhttp_request_free(p->req);
    }
}
static duk_ret_t native_http_cb(duk_context *ctx)
{
    struct evhttp_request *req = duk_require_pointer(ctx, 0);
    http_request_t *p = duk_require_pointer(ctx, 1);
    duk_pop_2(ctx);
    if (p->hasErr)
    {
        switch (p->err)
        {
        case EVREQ_HTTP_TIMEOUT:
            duk_error(ctx, DUK_ERR_ERROR, "Timeout.");
            break;
        case EVREQ_HTTP_EOF:
            duk_error(ctx, DUK_ERR_ERROR, "EOF reached.");
            break;
        case EVREQ_HTTP_INVALID_HEADER:
            duk_error(ctx, DUK_ERR_ERROR, "Error while reading header, or invalid header.");
            break;
        case EVREQ_HTTP_BUFFER_ERROR:
            duk_error(ctx, DUK_ERR_ERROR, "Error encountered while reading or writing.");
            break;
        case EVREQ_HTTP_REQUEST_CANCEL:
            duk_error(ctx, DUK_ERR_ERROR, "The evhttp_cancel_request() called on this request.");
            break;
        case EVREQ_HTTP_DATA_TOO_LONG:
            duk_error(ctx, DUK_ERR_ERROR, "Body is greater then evhttp_connection_set_max_body_size().");
            break;
        default:
            duk_error(ctx, DUK_ERR_ERROR, "Unknow error.");
            break;
        }
    }
    if (!req)
    {
        duk_error(ctx, DUK_ERR_ERROR, "connect server error");
    }
    int code = evhttp_request_get_response_code(req);
    if (!code)
    {
        duk_error(ctx, DUK_ERR_ERROR, "response code 0");
    }
    duk_push_object(ctx);
    duk_push_int(ctx, code);
    duk_put_prop_lstring(ctx, -2, "code", 4);
    struct evkeyvalq *header = evhttp_request_get_input_headers(req);
    duk_push_object(ctx);
    if (header)
    {
        for (struct evkeyval *node = header->tqh_first; node; node = node->next.tqe_next)
        {
            duk_push_string(ctx, node->key);
            duk_push_string(ctx, node->value);
            duk_put_prop(ctx, -3);
        }
    }
    duk_put_prop_lstring(ctx, -2, "header", 6);
    struct evbuffer *buffer = evhttp_request_get_input_buffer(req);
    if (buffer)
    {
        size_t n = evbuffer_get_length(buffer);
        if (n > 0)
        {
            void *dst = duk_push_buffer(ctx, n, 0);
            if (evbuffer_remove(buffer, dst, n) < 0)
            {
                duk_error(ctx, DUK_ERR_ERROR, "read response.body error");
            }
            duk_put_prop_lstring(ctx, -2, "body", 4);
        }
        else if (n > 1024 * 1024 * 5)
        {
            duk_range_error(ctx, "response.body too large %d", n);
        }
    }
    return 1;
}
static void on_http_cb(struct evhttp_request *req, void *arg)
{
    http_request_t *p = arg;
    if (!p->vm)
    {
        return;
    }
    duk_context *ctx = p->vm->ctx;
    duk_push_c_lightfunc(ctx, native_http_cb, 2, 2, 0);
    duk_push_pointer(ctx, req);
    duk_push_pointer(ctx, arg);
    if (duk_pcall(ctx, 2))
    {
        vm_async_reject(ctx, p);
        return;
    }
    vm_async_resolve(ctx, p);
}
static void on_http_error(enum evhttp_request_error err, void *arg)
{
    http_request_t *p = arg;
    p->err = err;
    p->hasErr = 1;
}
static duk_ret_t _native_free_request(duk_context *ctx)
{
    vm_finalizer_free(ctx, 0, http_request_free);
    return 0;
}
static duk_ret_t _native_new_request(duk_context *ctx)
{
    duk_size_t sz = 0;
    const char *body;
    if (duk_is_string(ctx, 0))
    {
        body = duk_require_lstring(ctx, 0, &sz);
    }
    else if (duk_is_buffer_data(ctx, 0))
    {
        body = duk_get_buffer_data(ctx, 0, &sz);
    }
    else if (duk_is_null_or_undefined(ctx, 0))
    {
        duk_pop(ctx);
    }
    else
    {
        duk_type_error(ctx, "body invalid");
    }

    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(http_request_t));
    finalizer->free = http_request_free;
    http_request_t *p = finalizer->p;

    p->req = evhttp_request_new(on_http_cb, p);
    if (!p->req)
    {
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_request_new error");
    }
    evhttp_request_set_error_cb(p->req, on_http_error);

    if (sz)
    {
        struct evbuffer *buf = evhttp_request_get_output_buffer(p->req);
        if (!buf)
        {
            duk_error(ctx, DUK_ERR_ERROR, "evhttp_request_get_output_buffer error");
        }
        if (evbuffer_add(buf, body, sz))
        {
            duk_error(ctx, DUK_ERR_ERROR, "evbuffer_add error");
        }
    }
    return 1;
}
static duk_ret_t _native_add_header(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_request_free);
    http_request_t *p = finalizer->p;
    if (!p->req)
    {
        duk_type_error(ctx, "req invalid");
    }
    const char *key = duk_require_string(ctx, 1);
    const char *value = duk_require_string(ctx, 2);

    struct evkeyvalq *headers = evhttp_request_get_output_headers(p->req);
    if (!headers)
    {
        duk_type_error(ctx, "evhttp_request_get_output_headers error");
    }
    if (evhttp_add_header(headers, key, value))
    {
        duk_type_error(ctx, "evhttp_add_header error");
    }
    return 0;
}
static enum evhttp_cmd_type _native_check_method(duk_context *ctx, const char *method, duk_size_t sz)
{
    switch (sz)
    {
    case 3:
        if (!memcmp(method, "GET", sz))
        {
            return EVHTTP_REQ_GET;
        }
        else if (!memcmp(method, "PUT", sz))
        {
            return EVHTTP_REQ_PUT;
        }
        break;
    case 4:
        if (!memcmp(method, "POST", sz))
        {
            return EVHTTP_REQ_POST;
        }
        else if (!memcmp(method, "HEAD", sz))
        {
            return EVHTTP_REQ_HEAD;
        }
        break;
    case 5:
        if (!memcmp(method, "PATCH", sz))
        {
            return EVHTTP_REQ_PATCH;
        }
        break;
    case 6:
        if (!memcmp(method, "DELETE", sz))
        {
            return EVHTTP_REQ_DELETE;
        }
        break;
    default:
        break;
    }
    duk_type_error(ctx, "method invalid");
    return EVHTTP_REQ_GET;
}
static duk_ret_t _native_make_request(duk_context *ctx)
{

    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_conn_free);
    http_conn_t *conn = finalizer->p;
    finalizer = vm_require_finalizer(ctx, 1, http_request_free);
    http_request_t *req = finalizer->p;
    duk_size_t sz_method;
    const char *method = duk_require_lstring(ctx, 2, &sz_method);
    enum evhttp_cmd_type type = _native_check_method(ctx, method, sz_method);
    const char *path = duk_require_string(ctx, 3);

    // push args
    duk_push_object(ctx);
    duk_swap_top(ctx, 0);
    duk_put_prop_lstring(ctx, 0, "finalizer", 9);
    duk_put_prop_lstring(ctx, 0, "path", 4);
    duk_pop(ctx);
    duk_put_prop_lstring(ctx, 0, "conn", 4);

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_COMPLETER);

    // [args,  completer]
    struct evhttp_request *request = req->req;
    req->req = NULL;
    if (evhttp_make_request(conn->conn, request, type, path))
    {
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_make_request error");
    }
    vm_async_completer_args(ctx, req);
    req->vm = vm;
    return 1;
}

static duk_ret_t _native_ws_key(duk_context *ctx)
{
    duk_uint8_t *key = duk_push_buffer(ctx, 16, 0);
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < 16; i++)
    {
        key[i] = rand() % 255;
    }
    duk_base64_encode(ctx, -1);
    return 1;
}
typedef struct
{
    vm_context_t *vm;
    struct event *timeout;
    struct bufferevent *bev;
    SSL_CTX *sctx;
    int step;
    int err;

    char *buf;
    size_t sz_buf;
    duk_uint32_t flags;
} websocket_connect_t;
static void websocket_connect_free(void *p)
{
    websocket_connect_t *ws = p;
    puts(" --------- websocket_connect_free");
#ifdef VM_TRACE_FINALIZER
    puts(" --------- websocket_connect_free");
#endif
    if (ws->timeout)
    {
        event_free(ws->timeout);
    }
    if (ws->bev)
    {
        bufferevent_free(ws->bev);
    }
    if (ws->buf)
    {
        vm_free(ws->buf);
    }
    if (ws->sctx)
    {
        SSL_CTX_free(ws->sctx);
    }
    if (ws->vm)
    {
        vm_free_dns(ws->vm);
    }
}

static duk_ret_t _native_websocket_connect_event_cb_error(duk_context *ctx)
{
    websocket_connect_t *ws = duk_require_pointer(ctx, 0);
    duk_pop(ctx);
    vm_restore(ctx, VM_SNAPSHOT_WEBSOCKET, ws, 1);
    int err = ws->err;
    switch (err)
    {
    case _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc failed");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_ERROR_TIMEOUT:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "connect timeout");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_ERROR_SOCKET:
        err = bufferevent_socket_get_dns_error(ws->bev);
        if (err)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, evutil_gai_strerror(err));
        }
        else
        {
            err = EVUTIL_SOCKET_ERROR();
            if (err)
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, evutil_socket_error_to_string(err));
            }
            else
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "unknow error");
            }
        }
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_ERROR_ENABLE_READ:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "enable read");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_EOF:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "read eof");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_TIMEOUT:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "read timeout");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_ERROR:
        duk_push_error_object(ctx, DUK_ERR_ERROR, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_EOF:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "write on eof");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_TIMEOUT:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "write timeout");
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_ERROR:
        duk_push_error_object(ctx, DUK_ERR_ERROR, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        break;
    case _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_PROTOCOL_ERROR:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "handshake protocol not matched");
        break;
    default:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "unknow error");
        break;
    }

    vm_reject(ctx, -2);
    duk_pop_2(ctx);

    vm_finalizer_free(ctx, -1, websocket_connect_free);
    return 0;
}
static void websocket_connect_read_cb(struct bufferevent *bev, void *ctx)
{
    websocket_connect_t *ws = ctx;
    switch (ws->step)
    {
    case _VM_HTTP_WEBSOCKET_STEP_HANDSHAKE:
    {
        char *p = ws->buf;
        struct evbuffer *buf = bufferevent_get_input(ws->bev);
        size_t sz;
        struct evbuffer_ptr found;
        int first = p ? 0 : 1;
        while (1)
        {
            found = evbuffer_search_eol(buf, 0, &sz, EVBUFFER_EOL_CRLF);
            if (found.pos < 0)
            {
                return;
            }
            else if (!found.pos)
            {
                if (first)
                {
                    ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_PROTOCOL_ERROR;
                    vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                    return;
                }
                evbuffer_drain(buf, sz);
                break;
            }
            first = 0;
            if (ws->sz_buf < found.pos)
            {
                p = vm_malloc(found.pos < 128 ? 128 : found.pos);
                if (!p)
                {
                    ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                    vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                    return;
                }
                if (ws->sz_buf)
                {
                    vm_free(ws->buf);
                }
                ws->buf = p;
            }

            evbuffer_remove(buf, p, found.pos);
            evbuffer_drain(buf, 2);

            p[found.pos] = 0;

            if (first)
            {
                if (found.pos < 12)
                {
                    ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                    vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                    return;
                }
                else if (found.pos == 12)
                {
                    if (memcpy("HTTP/1.1 101", p, 12))
                    {
                        ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                        vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                        return;
                    }
                }
                else if (memcpy("HTTP/1.1 101 ", p, 13))
                {
                    ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                    vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                    return;
                }
            }
            else
            {
                switch (found.pos)
                {
                case 18:
                    if (memcpy(p, "Upgrade: websocket", 18))
                    {
                        ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                        vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                        return;
                    }
                    ws->flags |= _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_UPGRADE;
                    break;
                case 19:
                    if (memcpy(p, "Connection: Upgrade", 19))
                    {
                        ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_MALLOC;
                        vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
                        return;
                    }
                    ws->flags |= _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_CONNECTION;
                    break;
                case 50:
                    if (!memcpy(p, "Sec-WebSocket-Accept: ", 22))
                    {
                        char sec[28] = {0};

                        ws->flags |= _VM_HTTP_WEBSOCKET_CONNECT_FLAGS_SEC;
                    }
                    break;
                }
            }
            printf("'%s' %ld\n", p, found.pos);
        }
        puts("ok");
        exit(1);
    }
    break;
    case _VM_HTTP_WEBSOCKET_STEP_READY:
        break;
    }
}
static void websocket_connect_timeout_cb(evutil_socket_t fd, short events, void *arg)
{
    websocket_connect_t *ws = arg;
    ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_TIMEOUT;
    vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
}
static void websocket_connect_event_cb(struct bufferevent *bev, short what, void *ctx)
{
    // puts("websocket_connect_event_cb");
    websocket_connect_t *ws = ctx;
    switch (ws->step) // 連接響應
    {
    case _VM_HTTP_WEBSOCKET_STEP_CONNECT:
        if (what & BEV_EVENT_CONNECTED) // tcp 連接成功
        {
            // 啓用 讀寫
            if (bufferevent_enable(ws->bev, EV_READ | EV_WRITE))
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_ENABLE_READ;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
            else
            {
                ws->step = _VM_HTTP_WEBSOCKET_STEP_HANDSHAKE;
            }
        }
        else
        {
            ws->err = _VM_HTTP_WEBSOCKET_CONNECT_ERROR_SOCKET;
            vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
        }
        return;
    case _VM_HTTP_WEBSOCKET_STEP_HANDSHAKE:
        if (what & BEV_EVENT_READING)
        {
            if (what & BEV_EVENT_EOF)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_EOF;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
            else if (what & BEV_EVENT_TIMEOUT)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_TIMEOUT;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
            else if (what & BEV_EVENT_ERROR)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_READ_ERROR;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
        }
        else if (what & BEV_EVENT_WRITING)
        {
            if (what & BEV_EVENT_EOF)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_EOF;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
            else if (what & BEV_EVENT_TIMEOUT)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_TIMEOUT;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
            else if (what & BEV_EVENT_ERROR)
            {
                ws->err = _VM_HTTP_WEBSOCKET_CONNECT_HANDSHAKE_WRITE_ERROR;
                vm_complete_lightfunc_noresult(ws->vm->ctx, _native_websocket_connect_event_cb_error, ws);
            }
        }
        return;
    }
    // _VM_HTTP_WEBSOCKET_STEP_READY
}
static duk_ret_t _native_ws_connect(duk_context *ctx)
{
    // duk_require_object(ctx, -1);
    VM_DUK_REQUIRE_AND_DEL_LSTRING(
        duk_bool_t wss = duk_require_boolean(ctx, -1),
        ctx, -1, "wss", 3)
    VM_DUK_REQUIRE_AND_DEL_LSTRING(
        int port = duk_require_number(ctx, -1),
        ctx, -1, "port", 4)
    VM_DUK_REQUIRE_LSTRING(
        const char *addr = duk_require_string(ctx, -1),
        ctx, -1, "addr", 4)
    VM_DUK_REQUIRE_AND_DEL_LSTRING(
        int timeout = duk_require_number(ctx, -1),
        ctx, -1, "timeout", 7)
    VM_DUK_REQUIRE_LSTRING(
        const char *key = duk_require_string(ctx, -1),
        ctx, -1, "key", 3)
    duk_size_t len;
    VM_DUK_REQUIRE_LSTRING(
        const char *handshake = duk_require_lstring(ctx, -1, &len),
        ctx, -1, "handshake", 9)

    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(websocket_connect_t));
    websocket_connect_t *ws = finalizer->p;
    finalizer->free = websocket_connect_free;

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    ws->vm = vm;
    vm->dns++;
    if (wss)
    {
        VM_DUK_REQUIRE_LSTRING(
            const char *hostname = duk_require_string(ctx, -1),
            ctx, 0, "hostname", 8)

        ws->sctx = SSL_CTX_new(TLS_client_method());
        if (!ws->sctx)
        {
            vm_finalizer_free(ctx, 1, websocket_connect_free);
            duk_error(ctx, DUK_ERR_ERROR, "SSL_CTX_new error");
        }
        SSL_CTX_set_timeout(ws->sctx, 3000);
        SSL_CTX_set_verify(ws->sctx, SSL_VERIFY_PEER, tls_verify_callback);
        SSL_CTX_set_default_verify_paths(ws->sctx);
        SSL *ssl = SSL_new(ws->sctx);
        if (!ssl)
        {
            vm_finalizer_free(ctx, 1, websocket_connect_free);
            duk_error(ctx, DUK_ERR_ERROR, "SSL_new error");
        }
        // set sni
        if (SSL_set_tlsext_host_name(ssl, hostname) != SSL_SUCCESS)
        {
            SSL_free(ssl);
            vm_finalizer_free(ctx, 1, websocket_connect_free);
            duk_error(ctx, DUK_ERR_ERROR, "SSL_set_tlsext_host_name error");
        }
        ws->bev = bufferevent_openssl_socket_new(vm->eb, -1, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    }
    else
    {
        ws->bev = bufferevent_socket_new(vm->eb, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    }

    bufferevent_setcb(ws->bev, websocket_connect_read_cb, NULL, websocket_connect_event_cb, ws);

    // 連接服務器
    if (bufferevent_socket_connect_hostname(ws->bev, vm->esb, AF_UNSPEC, addr, port))
    {
        vm_finalizer_free(ctx, 1, websocket_connect_free);
        duk_error(ctx, DUK_ERR_ERROR, "bufferevent_socket_connect_hostname error");
    }
    // 連接成功後自動發送 握手包
    if (bufferevent_write(ws->bev, handshake, len))
    {
        vm_finalizer_free(ctx, 1, websocket_connect_free);
        duk_error(ctx, DUK_ERR_ERROR, "bufferevent_write handshake error");
    }
    if (timeout > 0)
    {
        ws->timeout = event_new(vm->eb, -1, EV_TIMEOUT, websocket_connect_timeout_cb, ws);
        if (!ws->timeout)
        {
            vm_finalizer_free(ctx, 1, websocket_connect_free);
            duk_error(ctx, DUK_ERR_ERROR, "event_new connect timeout error");
        }
        struct timeval tv = {
            .tv_sec = timeout / 1000,
            .tv_usec = (timeout % 1000) * 1000,
        };
        if (event_add(ws->timeout, &tv))
        {
            vm_finalizer_free(ctx, 1, websocket_connect_free);
            duk_error(ctx, DUK_ERR_ERROR, "event_add connect timeout error");
        }
    }

    // opts, finalizer
    vm_new_completer(ctx, VM_SNAPSHOT_WEBSOCKET, ws, 2);
    return 1;
}

duk_ret_t native_iotjs_net_http_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, js_iotjs_modules_js_net_http_min_js, js_iotjs_modules_js_net_http_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_remove(ctx, -2);
    duk_push_object(ctx);
    {
        duk_push_c_function(ctx, _native_uri_parse, 1);
        duk_put_prop_lstring(ctx, -2, "uri_parse", 9);
        duk_push_c_function(ctx, _native_connect, 3);
        duk_put_prop_lstring(ctx, -2, "connect", 7);
        duk_push_c_function(ctx, _native_free_connect, 1);
        duk_put_prop_lstring(ctx, -2, "free_connect", 12);
        duk_push_c_function(ctx, _native_is_close_conn, 1);
        duk_put_prop_lstring(ctx, -2, "is_close_conn", 13);
        duk_push_c_function(ctx, _native_new_request, 1);
        duk_put_prop_lstring(ctx, -2, "new_request", 11);
        duk_push_c_function(ctx, _native_free_request, 1);
        duk_put_prop_lstring(ctx, -2, "free_request", 12);
        duk_push_c_function(ctx, _native_add_header, 3);
        duk_put_prop_lstring(ctx, -2, "add_header", 10);
        duk_push_c_function(ctx, _native_make_request, 4);
        duk_put_prop_lstring(ctx, -2, "make_request", 12);

        duk_push_c_function(ctx, _native_ws_key, 0);
        duk_put_prop_lstring(ctx, -2, "ws_key", 6);
        duk_push_c_function(ctx, _native_ws_connect, 1);
        duk_put_prop_lstring(ctx, -2, "ws_connect", 10);
    }
    duk_call(ctx, 3);
    return 0;
}