#include <iotjs/core/configure.h>
#include <iotjs/core/defines.h>
#include <iotjs/core/memory.h>
#include <iotjs/modules/module.h>
#include <iotjs/core/finalizer.h>
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

#include <iotjs/modules/native_http.h>

typedef struct
{
    vm_context_t *vm;
    struct evhttp_connection *conn;
    SSL_CTX *ssl_ctx;
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
    }
    if (p->ssl_ctx)
    {
        SSL_CTX_free(p->ssl_ctx);
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
    VM_DUK_REQUIRE_LSTRING(
        const char *addr = duk_require_string(ctx, -1),
        ctx, 0, "addr", 4)
    VM_DUK_REQUIRE_LSTRING(
        int port = duk_require_number(ctx, -1),
        ctx, 0, "port", 4)
    VM_DUK_REQUIRE_LSTRING(
        duk_bool_t tls = duk_require_boolean(ctx, -1),
        ctx, 0, "tls", 3)

    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(http_conn_t));
    finalizer->free = http_conn_free;
    http_conn_t *p = finalizer->p;

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    p->vm = vm;
    vm->dns++;
    struct bufferevent *bev;
    if (tls)
    {
        VM_DUK_REQUIRE_LSTRING(
            const char *hostname = duk_require_string(ctx, -1),
            ctx, 0, "hostname", 8)
        VM_DUK_REQUIRE_LSTRING(
            duk_bool_t insecure = duk_require_boolean(ctx, -1),
            ctx, 0, "insecure", 8)

        p->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!p->ssl_ctx)
        {
            vm_finalizer_free(ctx, 1, http_conn_free);
            duk_push_lstring(ctx, "SSL_CTX_new fail", 16);
            duk_throw(ctx);
        }
        if (insecure)
        {
            SSL_CTX_set_verify(p->ssl_ctx, SSL_VERIFY_PEER, tls_verify_callback);
        }
        SSL_CTX_set_default_verify_paths(p->ssl_ctx);
        SSL *ssl = SSL_new(p->ssl_ctx);
        if (!ssl)
        {
            vm_finalizer_free(ctx, 1, http_conn_free);
            duk_push_lstring(ctx, "SSL_new fail", 12);
            duk_throw(ctx);
        }
        // set sni
        if (SSL_set_tlsext_host_name(ssl, hostname) != SSL_SUCCESS)
        {
            SSL_free(ssl);
            vm_finalizer_free(ctx, 1, http_conn_free);
            duk_push_lstring(ctx, "SSL_set_tlsext_host_name fail", 29);
            duk_throw(ctx);
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

    p->conn = evhttp_connection_base_bufferevent_new(vm->eb, vm->esb, bev, addr, port);
    if (!p->conn)
    {
        bufferevent_free(bev);
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_connection_base_bufferevent_new error");
    }
    evhttp_connection_set_closecb(p->conn, http_conn_on_close, p);

    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_dup_top(ctx);

    // opts, cb, finalizer
    duk_context *snapshot = vm_snapshot(ctx, VM_SNAPSHOT_HTTP_CONN, p, 1);
    return 1;
}
static duk_ret_t _native_close(duk_context *ctx)
{
    finalizer_t *finalizer = vm_finalizer_free(ctx, 0, http_conn_free);
    vm_remove_snapshot(ctx, VM_SNAPSHOT_HTTP_CONN, finalizer->p);
    return 0;
}

typedef struct
{
    vm_context_t *vm;
    struct evhttp_request *req;
    struct evhttp_request *req0;
    enum evhttp_request_error err;
    duk_uint64_t limit;
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
    vm_restore(ctx, VM_SNAPSHOT_HTTP_REQUEST, p, 1);
    vm_finalizer_free(ctx, -1, http_request_free);
    if (duk_get_top(ctx) == 2)
    {
        duk_pop(ctx);
    }
    else
    {
        duk_swap(ctx, 0, 1);
        duk_pop_2(ctx);
    }
    if (p->hasErr)
    {
        switch (p->err)
        {
        case EVREQ_HTTP_TIMEOUT:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "Timeout.", 8);
            duk_call(ctx, 2);
            return 0;
        case EVREQ_HTTP_EOF:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "EOF reached.", 12);
            duk_call(ctx, 2);
            return 0;
        case EVREQ_HTTP_INVALID_HEADER:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "Error while reading header, or invalid header.", 46);
            duk_call(ctx, 2);
            return 0;
        case EVREQ_HTTP_BUFFER_ERROR:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "Error encountered while reading or writing.", 43);
            duk_call(ctx, 2);
            return 0;
        case EVREQ_HTTP_REQUEST_CANCEL:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "The evhttp_cancel_request() called on this request.", 51);
            duk_call(ctx, 2);
            return 0;
        case EVREQ_HTTP_DATA_TOO_LONG:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "Body is greater then evhttp_connection_set_max_body_size().", 59);
            duk_call(ctx, 2);
            return 0;
        default:
            duk_push_undefined(ctx);
            duk_push_lstring(ctx, "Unknow error.", 13);
            duk_call(ctx, 2);
            return 0;
        }
    }
    if (!req)
    {
        duk_push_undefined(ctx);
        duk_push_lstring(ctx, "Connect error.", 14);
        duk_call(ctx, 2);
        return 0;
    }
    int code = evhttp_request_get_response_code(req);
    if (!code)
    {
        duk_push_undefined(ctx);
        duk_push_lstring(ctx, "Response code 0", 15);
        duk_call(ctx, 2);
        return 0;
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
        else if (p->limit && n > p->limit)
        {
            duk_range_error(ctx, "response.body too large %d", n);
        }
    }
    duk_call(ctx, 1);
    return 0;
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
    duk_call(ctx, 2);
    duk_pop(ctx);
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
    duk_require_callable(ctx, 1);
    duk_size_t sz = 0;
    const char *body;
    duk_bool_t undefined = 0;
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
        undefined = 1;
        duk_swap_top(ctx, -2);
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
        duk_push_lstring(ctx, "evhttp_request_new error", 24);
        duk_throw(ctx);
    }
    p->req0 = p->req;
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

    vm_snapshot_copy(ctx, VM_SNAPSHOT_HTTP_REQUEST, p, undefined ? 2 : 3);
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

    duk_size_t sz_method;
    VM_DUK_REQUIRE_LSTRING(
        const char *method = duk_require_lstring(ctx, -1, &sz_method),
        ctx, 0, "method", 6)
    enum evhttp_cmd_type type = _native_check_method(ctx, method, sz_method);
    VM_DUK_REQUIRE_LSTRING(
        const char *path = duk_require_string(ctx, -1),
        ctx, 0, "path", 4)
    VM_DUK_REQUIRE_LSTRING(
        duk_uint64_t limit = (duk_uint64_t)duk_require_number(ctx, -1),
        ctx, 0, "limit", 5)

    duk_get_prop_lstring(ctx, 0, "conn", 4);
    finalizer_t *finalizer = vm_require_finalizer(ctx, -1, http_conn_free);
    http_conn_t *conn = finalizer->p;
    duk_pop(ctx);

    duk_get_prop_lstring(ctx, 0, "req", 3);
    finalizer = vm_require_finalizer(ctx, -1, http_request_free);
    http_request_t *req = finalizer->p;
    req->limit = limit;
    duk_pop(ctx);

    vm_context_t *vm = vm_get_context(ctx);
    struct evhttp_request *request = req->req;
    req->req = NULL;
    if (evhttp_make_request(conn->conn, request, type, path))
    {
        duk_error(ctx, DUK_ERR_ERROR, "evhttp_make_request error");
    }
    req->vm = vm;
    return 0;
}
static duk_ret_t _native_cancel_request(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_request_free);
    http_request_t *req = finalizer->p;
    evhttp_cancel_request(req->req0);
    return 0;
}
static duk_ret_t _native_set_priority(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_conn_free);
    http_conn_t *p = finalizer->p;
    duk_int_t v = duk_require_int(ctx, 1);
    bufferevent_priority_set(evhttp_connection_get_bufferevent(p->conn), v);
    return 0;
}
static duk_ret_t _native_get_priority(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, http_conn_free);
    http_conn_t *p = finalizer->p;
    duk_pop(ctx);
    int v = bufferevent_get_priority(evhttp_connection_get_bufferevent(p->conn));
    duk_push_number(ctx, v);
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
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_object(ctx);
    {
        duk_push_c_lightfunc(ctx, _native_connect, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "connect", 7);
        duk_push_c_lightfunc(ctx, _native_close, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "close", 5);
        duk_push_c_lightfunc(ctx, _native_new_request, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "new_request", 11);
        duk_push_c_lightfunc(ctx, _native_free_request, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "free_request", 12);
        duk_push_c_lightfunc(ctx, _native_add_header, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "add_header", 10);
        duk_push_c_lightfunc(ctx, _native_make_request, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "make_request", 12);
        duk_push_c_lightfunc(ctx, _native_cancel_request, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "cancel_request", 14);

        duk_push_c_lightfunc(ctx, _native_set_priority, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "setPriority", 11);
        duk_push_c_lightfunc(ctx, _native_get_priority, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "getPriority", 11);

        native_iotjs_net_deps_http(ctx, 1);
    }
    duk_call(ctx, 3);
    return 0;
}