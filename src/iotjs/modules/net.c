
#include <iotjs/modules/module.h>
#include <iotjs/modules/net.h>
#include <iotjs/modules/js/tsc.net.h>
#include <iotjs/core/configure.h>
#include <iotjs/core/memory.h>
duk_ret_t native_get_binary_length(duk_context *ctx)
{
    duk_size_t sz;
    if (duk_is_string(ctx, 0))
    {
        sz = duk_get_length(ctx, 0);
    }
    else
    {
        duk_get_buffer_data(ctx, 0, &sz);
    }
    duk_pop(ctx);
    duk_push_number(ctx, sz);
    return 1;
}
duk_ret_t native_socket_error(duk_context *ctx)
{
    duk_push_string(ctx, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
    return 1;
}
/***    BEGIN tcp   ***/
#define IOTJS_NET_TCPCONN_CONNECT_SUCCESS 0
#define IOTJS_NET_TCPCONN_CONNECT_TIMEOUT 10
#define IOTJS_NET_TCPCONN_CONNECT_ERROR 11
static int tls_verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    return 1;
}
static void tcp_connection_free(void *p)
{
    tcp_connection_t *conn = p;
#ifdef VM_TRACE_FINALIZER
    puts(" --------- tcp_connection_free");
#endif
    if (conn->timeout)
    {
        event_free(conn->timeout);
    }
    if (conn->bev)
    {
        bufferevent_free(conn->bev);
    }
    if (conn->ssl_ctx)
    {
        SSL_CTX_free(conn->ssl_ctx);
    }
    if (conn->vm)
    {
        vm_free_dns(conn->vm);
    }
}
static duk_ret_t native_tcp_connect_cb(duk_context *ctx)
{
    tcp_connection_t *conn = duk_require_pointer(ctx, 0);
    duk_int_t err = -1989;
    if (duk_is_number(ctx, 1))
    {
        err = duk_require_int(ctx, 1);
        duk_pop_2(ctx);
    }
    else
    {
        duk_remove(ctx, 0);
    }
    duk_context *snapshot = vm_restore(ctx, VM_SNAPSHOT_TCPCONN, conn, err == IOTJS_NET_TCPCONN_CONNECT_SUCCESS ? 0 : 1);
    switch (err)
    {
    case IOTJS_NET_TCPCONN_CONNECT_SUCCESS:
        duk_call(ctx, 1);

        duk_swap_top(snapshot, 0);
        duk_pop_2(snapshot);
        return 0;
    case -1989:
        duk_swap_top(ctx, -2);
        duk_push_undefined(ctx);
        duk_dup(ctx, 0);
        break;
    case IOTJS_NET_TCPCONN_CONNECT_TIMEOUT:
        duk_swap_top(ctx, -2);
        duk_push_undefined(ctx);
        duk_push_lstring(ctx, "connect timeout", 15);
        break;
    case IOTJS_NET_TCPCONN_CONNECT_ERROR:
    {
        duk_swap_top(ctx, -2);
        duk_push_undefined(ctx);
        int e = bufferevent_socket_get_dns_error(conn->bev);
        if (e)
        {
            duk_push_string(ctx, evutil_gai_strerror(e));
        }
        else
        {
            e = EVUTIL_SOCKET_ERROR();
            if (e)
            {
                duk_push_string(ctx, evutil_socket_error_to_string(e));
            }
            else
            {
                duk_push_lstring(ctx, "unknow error", 12);
            }
        }
    }
    break;
    default:
        duk_swap_top(ctx, -2);
        duk_push_undefined(ctx);
        duk_push_lstring(ctx, "unknow error", 12);
        break;
    }
    duk_call(ctx, 2);
    duk_pop(ctx);
    vm_finalizer_free(ctx, -1, tcp_connection_free);
    return 0;
}
static void tcp_connection_connect_ec(tcp_connection_t *conn, duk_int_t err)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_tcp_connect_cb, 2, 2, 0);
    duk_push_pointer(ctx, conn);
    duk_push_int(ctx, err);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void tcp_connection_timeout_cb(evutil_socket_t fd, short events, void *args)
{
    tcp_connection_connect_ec(args, IOTJS_NET_TCPCONN_CONNECT_TIMEOUT);
}
static void tcp_connection_read_cb(struct bufferevent *bev, void *args)
{
    tcp_connection_t *conn = args;
    duk_context *ctx = conn->vm->ctx;
    duk_context *snapshot = vm_require_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn);
    duk_push_lstring(snapshot, "onRead", 6);
    duk_call_prop(snapshot, 0, 0);
    duk_pop(snapshot);
}
static void tcp_connection_write_cb(struct bufferevent *bev, void *args)
{
    tcp_connection_t *conn = args;
    duk_context *ctx = conn->vm->ctx;
    duk_context *snapshot = vm_require_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn);
    duk_push_lstring(snapshot, "onWrite", 7);
    duk_call_prop(snapshot, 0, 0);
    duk_pop(snapshot);
}

static void tcp_connection_event_cb(struct bufferevent *bev, short what, void *args)
{
    tcp_connection_t *conn = args;
    if (!conn->step)
    {
        if (what & BEV_EVENT_CONNECTED)
        {
            if (conn->timeout)
            {
                event_free(conn->timeout);
                conn->timeout = NULL;
            }
            tcp_connection_connect_ec(conn, IOTJS_NET_TCPCONN_CONNECT_SUCCESS);
            conn->step = 1;
        }
        else
        {
            tcp_connection_connect_ec(conn, IOTJS_NET_TCPCONN_CONNECT_ERROR);
        }
        return;
    }
    duk_context *ctx = conn->vm->ctx;
    duk_context *snapshot = vm_require_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn);
    duk_push_lstring(snapshot, "onEvent", 7);
    duk_push_number(snapshot, what);
    duk_call_prop(snapshot, 0, 1);
    duk_pop(snapshot);
}
static duk_ret_t native_tcp_connect(duk_context *ctx)
{
    duk_require_callable(ctx, 1);
    VM_DUK_REQUIRE_LSTRING(
        const char *addr = duk_require_string(ctx, -1),
        ctx, 0, "addr", 4)
    VM_DUK_REQUIRE_LSTRING(
        int port = duk_require_number(ctx, -1),
        ctx, 0, "port", 4)
    VM_DUK_REQUIRE_LSTRING(
        duk_bool_t tls = duk_require_boolean(ctx, -1),
        ctx, 0, "tls", 3)
    VM_DUK_REQUIRE_LSTRING(
        int timeout = duk_require_number(ctx, -1),
        ctx, 0, "timeout", 7)
    VM_DUK_REQUIRE_LSTRING(
        size_t read = duk_require_uint(ctx, -1),
        ctx, 0, "read", 4)
    VM_DUK_REQUIRE_LSTRING(
        size_t write = duk_require_number(ctx, -1),
        ctx, 0, "write", 5)

    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(tcp_connection_t));
    tcp_connection_t *conn = finalizer->p;
    finalizer->free = tcp_connection_free;

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    conn->vm = vm;
    vm->dns++;
    if (tls)
    {
        VM_DUK_REQUIRE_LSTRING(
            const char *hostname = duk_require_string(ctx, -1),
            ctx, 0, "hostname", 8)

        conn->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!conn->ssl_ctx)
        {
            vm_finalizer_free(ctx, 1, tcp_connection_free);
            duk_push_lstring(ctx, "SSL_CTX_new fail", 16);
            duk_throw(ctx);
        }
        if (timeout > 0)
        {
            SSL_CTX_set_timeout(conn->ssl_ctx, 3000);
        }
        SSL_CTX_set_verify(conn->ssl_ctx, SSL_VERIFY_PEER, tls_verify_callback);
        SSL_CTX_set_default_verify_paths(conn->ssl_ctx);
        SSL *ssl = SSL_new(conn->ssl_ctx);
        if (!ssl)
        {
            vm_finalizer_free(ctx, 1, tcp_connection_free);
            duk_push_lstring(ctx, "SSL_new fail", 12);
            duk_throw(ctx);
        }
        // set sni
        if (SSL_set_tlsext_host_name(ssl, hostname) != SSL_SUCCESS)
        {
            SSL_free(ssl);
            vm_finalizer_free(ctx, 1, tcp_connection_free);
            duk_push_lstring(ctx, "SSL_set_tlsext_host_name fail", 29);
            duk_throw(ctx);
        }
        conn->bev = bufferevent_openssl_socket_new(vm->eb, -1, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    }
    else
    {
        conn->bev = bufferevent_socket_new(vm->eb, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    }
    // 設置回調
    bufferevent_setcb(conn->bev, tcp_connection_read_cb, tcp_connection_write_cb, tcp_connection_event_cb, conn);
    // 設置水印
    bufferevent_setwatermark(conn->bev, EV_READ, 0, read);
    conn->write = write;

    // 啓用 寫 回調
    if (bufferevent_enable(conn->bev, EV_WRITE))
    {
        vm_finalizer_free(ctx, 1, tcp_connection_free);
        duk_push_lstring(ctx, "bufferevent_enable fail", 23);
        duk_throw(ctx);
    }

    // 連接服務器
    if (bufferevent_socket_connect_hostname(conn->bev, vm->esb, AF_UNSPEC, addr, port))
    {
        vm_finalizer_free(ctx, 1, tcp_connection_free);
        duk_push_lstring(ctx, "bufferevent_socket_connect_hostname fail", 40);
        duk_throw(ctx);
    }
    // 設置 超時
    if (timeout > 0)
    {
        conn->timeout = event_new(vm->eb, -1, EV_TIMEOUT, tcp_connection_timeout_cb, conn);
        if (!conn->timeout)
        {
            vm_finalizer_free(ctx, 1, tcp_connection_free);
            duk_push_lstring(ctx, "event_new connect timeout fail", 30);
            duk_throw(ctx);
        }
        struct timeval tv = {
            .tv_sec = timeout / 1000,
            .tv_usec = (timeout % 1000) * 1000,
        };
        if (event_add(conn->timeout, &tv))
        {
            vm_finalizer_free(ctx, 1, tcp_connection_free);
            duk_push_lstring(ctx, "event_add connect timeout fail", 30);
            duk_throw(ctx);
        }
    }

    // opts, cb, finalizer
    duk_context *snapshot = vm_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn, 3);
    return 1;
}
static duk_ret_t native_tcp_free(duk_context *ctx)
{
    finalizer_t *finalizer = vm_finalizer_free(ctx, 0, tcp_connection_free);
    vm_remove_snapshot(ctx, VM_SNAPSHOT_TCPCONN, finalizer->p);
    return 0;
}
static duk_ret_t native_tcp_write(duk_context *ctx)
{
    const char *data;
    duk_size_t sz;
    if (duk_is_string(ctx, 1))
    {
        data = duk_require_lstring(ctx, 1, &sz);
    }
    else
    {
        data = duk_require_buffer_data(ctx, 1, &sz);
    }
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    if (sz)
    {
        struct evbuffer *buf = bufferevent_get_output(p->bev);
        size_t len = evbuffer_get_length(buf);

        if (p->write)
        {
            if (sz > p->write)
            {
                duk_pop_2(ctx);
                duk_push_lstring(ctx, "data too large", 14);
                duk_throw(ctx);
            }
            else if (p->write - len < sz)
            {
                duk_pop_2(ctx);
                duk_push_number(ctx, 0);
                return 1;
            }
        }

        if (bufferevent_write(p->bev, data, sz))
        {
            duk_pop_2(ctx);
            duk_push_number(ctx, 0);
        }
        else
        {
            duk_pop_2(ctx);
            duk_push_number(ctx, sz);
        }
    }
    else
    {
        duk_pop_2(ctx);
        duk_push_number(ctx, 0);
    }
    return 1;
}
static duk_ret_t native_tcp_read(duk_context *ctx)
{
    duk_size_t sz;
    void *data = duk_require_buffer_data(ctx, 1, &sz);
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    if (sz)
    {
        sz = bufferevent_read(p->bev, data, sz);
    }
    duk_pop_2(ctx);
    duk_push_number(ctx, sz);
    return 1;
}
static duk_ret_t native_tcp_enable(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    duk_size_t flags = duk_require_number(ctx, 1);
    if (bufferevent_enable(p->bev, flags))
    {
        duk_pop_2(ctx);
        duk_push_lstring(ctx, "bufferevent_enable fail", 23);
        duk_throw(ctx);
    }
    return 0;
}
static duk_ret_t native_tcp_disable(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    duk_size_t flags = duk_require_number(ctx, 1);
    if (bufferevent_disable(p->bev, flags))
    {
        duk_pop_2(ctx);
        duk_push_lstring(ctx, "bufferevent_disable fail", 24);
        duk_throw(ctx);
    }
    return 0;
}
static duk_ret_t native_tcp_read_more(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    struct evbuffer *buf = bufferevent_get_input(p->bev);
    size_t len = evbuffer_get_length(buf);
    if (!len)
    {
        return 0;
    }
    duk_pop(ctx);
    if (len > 1024 * 32)
    {
        len = 1024 * 32;
    }
    void *data = duk_push_buffer(ctx, len, 0);
    evbuffer_remove(buf, data, len);
    return 1;
}
static duk_ret_t native_tcp_readable(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    duk_pop(ctx);
    tcp_connection_t *p = finalizer->p;
    struct evbuffer *buf = bufferevent_get_input(p->bev);
    size_t len = evbuffer_get_length(buf);
    if (len > 0)
    {
        duk_push_true(ctx);
    }
    else
    {
        duk_push_false(ctx);
    }
    return 1;
}
static duk_ret_t native_tcp_writable(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    duk_pop(ctx);
    tcp_connection_t *p = finalizer->p;
    if (p->write)
    {
        struct evbuffer *buf = bufferevent_get_output(p->bev);
        size_t len = evbuffer_get_length(buf);
        if (p->write > len)
        {
            duk_push_true(ctx);
        }
        else
        {
            duk_push_false(ctx);
        }
    }
    else
    {
        duk_push_true(ctx);
    }
    return 1;
}
static duk_ret_t native_tcp_set_buffer(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    duk_bool_t read = duk_require_boolean(ctx, 1);
    duk_size_t n = duk_require_number(ctx, 2);
    duk_pop_3(ctx);
    if (read)
    {
        bufferevent_setwatermark(p->bev, EV_READ, 0, n);
    }
    else
    {
        p->write = n;
    }
    return 0;
}
static duk_ret_t native_tcp_get_buffer(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    duk_bool_t read = duk_require_boolean(ctx, 1);
    duk_pop_2(ctx);
    if (read)
    {
        size_t n;
        bufferevent_getwatermark(p->bev, EV_READ, 0, &n);
        duk_push_number(ctx, n);
    }
    else
    {
        duk_push_number(ctx, p->write);
    }
    return 1;
}
duk_ret_t native_tcp_set_timeout(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *p = finalizer->p;
    duk_uint_t read = duk_require_uint(ctx, 1);
    duk_uint_t write = duk_require_uint(ctx, 2);
    duk_pop_3(ctx);
    if (read)
    {
        if (write)
        {
            struct timeval tv_read =
                {
                    .tv_sec = read / 1000,
                    .tv_usec = (read % 1000) * 1000,
                };
            struct timeval tv_write =
                {
                    .tv_sec = write / 1000,
                    .tv_usec = (write % 1000) * 1000,
                };
            bufferevent_set_timeouts(p->bev, &tv_read, &tv_write);
        }
        else
        {
            struct timeval tv =
                {
                    .tv_sec = read / 1000,
                    .tv_usec = (read % 1000) * 1000,
                };
            bufferevent_set_timeouts(p->bev, &tv, NULL);
        }
    }
    else if (write)
    {
        struct timeval tv =
            {
                .tv_sec = write / 1000,
                .tv_usec = (write % 1000) * 1000,
            };
        bufferevent_set_timeouts(p->bev, NULL, &tv);
    }
    else
    {
        bufferevent_set_timeouts(p->bev, NULL, NULL);
    }
    return 0;
}

duk_ret_t native_iotjs_net_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, (const char *)iotjs_modules_js_tsc_net_min_js, iotjs_modules_js_tsc_net_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_remove(ctx, -2);
    duk_push_object(ctx);
    {
        duk_push_c_lightfunc(ctx, native_get_binary_length, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "get_length", 10);

        duk_push_c_lightfunc(ctx, native_tcp_connect, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_connect", 11);
        duk_push_c_lightfunc(ctx, native_tcp_free, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_free", 8);
        duk_push_c_lightfunc(ctx, native_tcp_write, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_write", 9);
        duk_push_c_lightfunc(ctx, native_tcp_read, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_read", 8);
        duk_push_c_lightfunc(ctx, native_tcp_enable, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_enable", 10);
        duk_push_c_lightfunc(ctx, native_tcp_disable, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_disable", 11);
        duk_push_c_lightfunc(ctx, native_tcp_readable, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_readable", 12);
        duk_push_c_lightfunc(ctx, native_tcp_writable, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_writable", 12);
        duk_push_c_lightfunc(ctx, native_tcp_read_more, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_readMore", 12);
        duk_push_c_lightfunc(ctx, native_tcp_set_buffer, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_setBuffer", 13);
        duk_push_c_lightfunc(ctx, native_tcp_get_buffer, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_getBuffer", 13);
        duk_push_c_lightfunc(ctx, native_tcp_set_timeout, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "tcp_setTimeout", 14);
    }
    duk_call(ctx, 3);
    return 0;
}