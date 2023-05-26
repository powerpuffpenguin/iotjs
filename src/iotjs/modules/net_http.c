#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

duk_ret_t _native_iotjs_net_http_test(duk_context *ctx)
{
    struct evkeyvalq querys;
    if (evhttp_parse_query_str("id=1&id=2&lv=3", &querys))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "evhttp_parse_query_str error");
        duk_throw(ctx);
    }
    duk_push_object(ctx);
    for (struct evkeyval *node = querys.tqh_first; node; node = node->next.tqe_next)
    {
        duk_push_string(ctx, node->key);
        duk_push_string(ctx, node->value);
        duk_put_prop(ctx, -3);
    }
    return 1;
}
duk_ret_t native_iotjs_net_http_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_push_c_function(ctx, _native_iotjs_net_http_test, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "http");
    return 0;
}