#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>

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

#include <iotjs/modules/js/tsc.net_http.h>
#include <string.h>
#include <stdlib.h>
static duk_ret_t _native_uri_finalizer(duk_context *ctx)
{
    duk_get_prop_lstring(ctx, 0, "ptr", 3);
    struct evhttp_uri *uri = duk_require_pointer(ctx, -1);
    evhttp_uri_free(uri);
}
static duk_ret_t _native_uri_parse(duk_context *ctx)
{
    const char *url = duk_require_string(ctx, 0);
    struct evhttp_uri *uri = evhttp_uri_parse(url);
    if (!uri)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "parse url error");
        duk_throw(ctx);
    }
    duk_pop(ctx);

    if (!duk_check_stack(ctx, 3))
    {
        evhttp_uri_free(uri);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "check_stack error");
        duk_throw(ctx);
    }
    duk_push_object(ctx);
    duk_push_pointer(ctx, uri);
    duk_put_prop_lstring(ctx, -2, "ptr", 3);
    duk_push_c_function(ctx, _native_uri_finalizer, 1);
    duk_set_finalizer(ctx, -2);

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

    duk_push_undefined(ctx);
    duk_set_finalizer(ctx, -2);
    duk_del_prop_lstring(ctx, -1, "ptr", 3);
    return 1;
}
typedef struct
{
    vm_context_t *vm;
    struct bufferevent *bev;
    struct evhttp_connection *conn;
    SSL_CTX *sctx;
    SSL *ssl;
    duk_uint8_t closed;
} http_conn_t;
static void http_conn_on_close(struct evhttp_connection *conn, void *arg)
{
    http_conn_t *p = arg;
    p->closed = 1;
}
static void http_conn_free(void *arg)
{
    http_conn_t *p = arg;
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
    vm_context_t *vm = p->vm;
    if (vm)
    {
        vm_free_dns(vm);
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
        p->ssl = SSL_new(p->sctx);
        if (!p->ssl)
        {
            duk_error(ctx, DUK_ERR_ERROR, "SSL_new error");
        }
        // set sni
        if (SSL_set_tlsext_host_name(p->ssl, host) != SSL_SUCCESS)
        {
            duk_error(ctx, DUK_ERR_ERROR, "SSL_set_tlsext_host_name error");
        }
        p->bev = bufferevent_openssl_socket_new(vm->eb, -1, p->ssl, BUFFEREVENT_SSL_CONNECTING, 0);
        if (!p->bev)
        {
            duk_error(ctx, DUK_ERR_ERROR, "bufferevent_openssl_socket_new error");
        }
    }
    else
    {
        p->bev = bufferevent_socket_new(vm->eb, -1, 0);
        if (!p->bev)
        {
            duk_error(ctx, DUK_ERR_ERROR, "bufferevent_socket_new error");
        }
    }

    p->conn = evhttp_connection_base_bufferevent_new(vm->eb, vm->esb, p->bev, host, port);
    if (!p->conn)
    {
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_connection_base_bufferevent_new error");
    }
    evhttp_connection_set_closecb(p->conn, http_conn_on_close, p);
    return 1;
}
typedef struct
{
    duk_context *vm;
    struct evhttp_request *req;
    enum evhttp_request_error err;
} http_request_t;
void http_request_free(void *arg)
{
    http_request_t *p = arg;
    if (p->req)
    {
        evhttp_request_free(p->req);
    }
}
void on_http_cb(struct evhttp_request *req, void *arg)
{
    http_request_t *p = arg;
    if (!p->vm)
    {
        return;
    }
    if (!req)
    {
        return;
    }
    int code = evhttp_request_get_response_code(req);
    if (!code)
    {
        return;
    }
}
void on_http_error(enum evhttp_request_error err, void *arg)
{
    http_request_t *p = arg;
    p->err = err;
}
duk_ret_t _native_new_request(duk_context *ctx)
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
    else if (!duk_is_null_or_undefined(ctx, 0))
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
duk_ret_t _native_add_header(duk_context *ctx)
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
enum evhttp_cmd_type _native_check_method(duk_context *ctx, const char *method, duk_size_t sz)
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
duk_ret_t _native_make_request(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_conn_free);
    http_conn_t *conn = finalizer->p;
    finalizer = vm_require_finalizer(ctx, 1, http_request_free);
    http_request_t *req = finalizer->p;
    duk_size_t sz_method;
    const char *method = duk_require_lstring(ctx, 2, &sz_method);
    enum evhttp_cmd_type type = _native_check_method(ctx, method, sz_method);
    const char *path = duk_require_string(ctx, 3);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_COMPLETER);
    duk_new(ctx, 0);
    duk_get_prop_lstring(ctx, -1, "promise", 7);
    duk_swap_top(ctx, 0);
    duk_put_prop_lstring(ctx, -2, "conn", 4);
    duk_swap_top(ctx, -2);
    duk_swap_top(ctx, 1);
    duk_put_prop_lstring(ctx, -2, "req", 3);
    duk_swap(ctx, 0, 1);

    // [path, promise, completer]
    req->vm = vm;
    vm_dump_context_stdout(ctx);
    exit(1);
    if (evhttp_make_request(conn->conn, req->req, type, path))
    {
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_make_request error");
    }
    req->req = NULL;
    return 1;
}
duk_ret_t _native_iotjs_net_http_request(duk_context *ctx)
{
    // struct evkeyvalq querys;
    // if (evhttp_parse_query_str("id=1&id=2&lv=3", &querys))
    // {
    //     duk_push_error_object(ctx, DUK_ERR_ERROR, "evhttp_parse_query_str error");
    //     duk_throw(ctx);
    // }
    // duk_push_object(ctx);
    // for (struct evkeyval *node = querys.tqh_first; node; node = node->next.tqe_next)
    // {
    //     duk_push_string(ctx, node->key);
    //     duk_push_string(ctx, node->value);
    //     duk_put_prop(ctx, -3);
    // }
    // finalizer_t *finalizer = vm_create_finalizer(ctx);
    // finalizer->free = test_free;
    // finalizer->p = 123;
    // vm_dump_context_stdout(ctx);
    return 1;
}
duk_ret_t native_iotjs_net_http_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    // duk_push_c_function(ctx, _native_iotjs_net_http_request, 2);
    // duk_put_prop_string(ctx, -2, "test");

    duk_eval_lstring(ctx, iotjs_modules_js_tsc_net_http_min_js, iotjs_modules_js_tsc_net_http_min_js_len);
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
        duk_push_c_function(ctx, _native_new_request, 1);
        duk_put_prop_lstring(ctx, -2, "new_request", 11);
        duk_push_c_function(ctx, _native_add_header, 3);
        duk_put_prop_lstring(ctx, -2, "add_header", 10);
        duk_push_c_function(ctx, _native_make_request, 4);
        duk_put_prop_lstring(ctx, -2, "make_request", 12);
    }
    duk_call(ctx, 3);
    return 0;
}