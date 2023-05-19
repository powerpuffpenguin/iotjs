
#include <iotjs/core/js.h>
duk_ret_t native_iotjs_net_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);
    return 0;
}