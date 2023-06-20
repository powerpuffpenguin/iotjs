#include <duktape.h>

duk_ret_t native_iotjs_mtd_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    // duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_net_min_js, js_iotjs_modules_js_net_min_js_len);
    // duk_swap_top(ctx, -2);
    // duk_push_heap_stash(ctx);
    // duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    // duk_swap_top(ctx, -2);
    // duk_pop(ctx);
    // duk_push_object(ctx);
    // {
    //     duk_push_c_lightfunc(ctx, native_get_binary_length, 1, 1, 0);
    //     duk_put_prop_lstring(ctx, -2, "get_length", 10);
    //     duk_push_c_lightfunc(ctx, native_socket_error, 0, 0, 0);
    //     duk_put_prop_lstring(ctx, -2, "socket_error", 12);
    //     // tcp conn
    //     duk_push_c_lightfunc(ctx, native_tcp_connect, 2, 2, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_connect", 11);
    //     duk_push_c_lightfunc(ctx, native_tcp_free, 1, 1, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_free", 8);
    //     duk_push_c_lightfunc(ctx, native_tcp_write, 2, 2, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_write", 9);
    //     duk_push_c_lightfunc(ctx, native_tcp_read, 2, 2, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_read", 8);
    //     duk_push_c_lightfunc(ctx, native_tcp_readable, 1, 1, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_readable", 12);
    //     duk_push_c_lightfunc(ctx, native_tcp_writable, 1, 1, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_writable", 12);
    //     duk_push_c_lightfunc(ctx, native_tcp_read_more, 1, 1, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_readMore", 12);
    //     duk_push_c_lightfunc(ctx, native_tcp_set_buffer, 3, 3, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_setBuffer", 13);
    //     duk_push_c_lightfunc(ctx, native_tcp_get_buffer, 2, 2, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_getBuffer", 13);
    //     duk_push_c_lightfunc(ctx, native_tcp_set_timeout, 3, 3, 0);
    //     duk_put_prop_lstring(ctx, -2, "tcp_setTimeout", 14);
    //     // websocket
    //     native_iotjs_net_deps_http(ctx, 0);
    // }
    duk_call(ctx, 3);
    return 0;
}