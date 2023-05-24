#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <event2/http.h>
void evhttp_request_cb(struct evhttp_request *req, void *arg)
{
    puts("evhttp_request_cb");
}
duk_ret_t _native_iotjs_net_http_finalizer(duk_context *ctx)
{
    puts("finalizer");
    return 0;
}
typedef struct
{
    vm_context_t *vm;
    void *heapptr;
} http_timer_t;

void timer_cb(evutil_socket_t fs, short events, void *arg)
{
    puts("timer_cb");
    http_timer_t *t = arg;
    duk_context *ctx = t->vm->ctx;
    duk_push_heapptr(ctx, t->heapptr);
    duk_push_string(ctx, "ok");
    // duk_push_string(ctx, "yes");
    duk_get_prop_string(ctx, -2, "v");
    duk_call_prop(ctx, -3, 1);
    duk_pop_2(ctx);
    vm_dump_context_stdout(ctx);
}
duk_ret_t _native_iotjs_net_http_new(duk_context *ctx)
{
    puts("_native_iotjs_net_http_new");

    duk_push_thread(ctx);
    duk_context *new_ctx = duk_get_context(ctx, -1 /*index*/);

    // duk_push_object(ctx);
    // duk_swap_top(ctx, 0);
    // duk_put_prop_string(ctx, 0, "v");
    // duk_put_prop_string(ctx, 0, "err");
    // duk_put_prop_string(ctx, 0, "ok");
    // duk_push_c_function(ctx, _native_iotjs_net_http_finalizer, 1);
    // duk_set_finalizer(ctx, -2);

    // void *heapptr = duk_get_heapptr(ctx, -1);

    // vm_context_t *vm = vm_get_context(ctx);
    // struct timeval tv = {
    //     .tv_sec = 1,
    //     .tv_usec = 0,
    // };
    // http_timer_t *t = malloc(sizeof(http_timer_t));
    // t->vm = vm;
    // t->heapptr = heapptr;
    // if (event_base_once(vm->eb, -1, EV_TIMEOUT, timer_cb, t, &tv))
    // {
    //     duk_push_string(ctx, "event_base_once error");
    //     duk_throw(ctx);
    // }

    // vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    // struct evhttp_connection *conn = evhttp_connection_base_bufferevent_new(vm->eb, vm->esb, NULL, "localhost", 80);
    // struct evhttp_request *req = evhttp_request_new(evhttp_request_cb, conn);

    // evhttp_make_request(conn, req, EVHTTP_REQ_GET, "http://127.0.0.1");
    return 1;
}
duk_ret_t _native_iotjs_net_http_call(duk_context *ctx)
{
    puts("_native_iotjs_net_http_call");
    duk_push_string(ctx, "call err");
    duk_throw(ctx);
    return 1;
}
duk_ret_t _native_iotjs_net_http_puts(duk_context *ctx)
{
    duk_idx_t thr_idx = duk_push_thread(ctx);
    printf("%d\n", thr_idx);
    duk_context *new_ctx = duk_get_context(ctx, 0);
    duk_push_heap_stash(new_ctx);
    duk_push_c_function(ctx, _native_iotjs_net_http_call, 0);
    duk_call(ctx, 0);
    vm_dump_context_stdout(new_ctx);
    return 0;
}
duk_ret_t native_iotjs_net_http_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_push_c_function(ctx, _native_iotjs_net_http_new, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "http");
    duk_push_c_function(ctx, _native_iotjs_net_http_puts, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "puts");
    return 0;
}