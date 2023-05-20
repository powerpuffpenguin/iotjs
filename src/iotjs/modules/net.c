
#include <iotjs/core/js.h>

duk_ret_t _buffer(duk_context *ctx)
{
    duk_push_buffer(ctx, 10, 0);
    vm_dump_context_stdout(ctx);

    return 1;
}
duk_ret_t native_iotjs_net_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);
    duk_push_c_function(ctx, _buffer, 0);
    duk_put_prop_string(ctx, -2, "buffer");
    return 0;
}