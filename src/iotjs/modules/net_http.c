#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <event2/http.h>
void evhttp_request_cb(struct evhttp_request *req, void *arg)
{
    puts("evhttp_request_cb");
}
duk_ret_t _native_iotjs_net_http_new(duk_context *ctx)
{
    puts("_native_iotjs_net_http_new");
    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB);
    struct evhttp_connection *conn = evhttp_connection_base_bufferevent_new(vm->eb, vm->esb, NULL, "localhost", 80);
    struct evhttp_request *req = evhttp_request_new(evhttp_request_cb, conn);

    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "http://127.0.0.1");
    return 0;
}
duk_ret_t native_iotjs_net_http_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_push_c_function(ctx, _native_iotjs_net_http_new, 0);
    duk_put_prop_string(ctx, -2, "http");
    return 0;
}