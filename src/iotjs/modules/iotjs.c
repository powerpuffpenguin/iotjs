#include <iotjs/core/vm.h>

duk_ret_t native_iotjs_init(duk_context *ctx)
{
    duk_push_string(ctx, VM_VERSION);
    duk_put_prop_string(ctx, 1, "version");

    return 0;
}
