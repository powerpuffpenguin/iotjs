#include <iotjs/core/vm.h>

duk_ret_t _native_iotjs_init(duk_context *ctx)
{
    duk_push_string(ctx, VM_VERSION);
    duk_put_prop_string(ctx, 1, "version");
    return 0;
}
__attribute((constructor)) void __iotjs_module_init()
{
    vm_register("iotjs", _native_iotjs_init);
}