#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <iotjs/core/defines.h>
duk_ret_t native_iotjs_init(duk_context *ctx)
{
    duk_swap_top(ctx, 0);
    duk_pop_2(ctx);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_get_prop_lstring(ctx, -2, VM_STASH_KEY_IOTJS);
    duk_swap_top(ctx, -3);
    duk_pop(ctx);

    // [module,iotjs,_iotjs]
    duk_get_prop_lstring(ctx, -1, "IotError", 8);
    duk_put_prop_lstring(ctx, -3, "IotError", 8);
    duk_pop(ctx);

    // [module,iotjs]
    duk_push_string(ctx, VM_IOTJS_OS);
    duk_put_prop_lstring(ctx, -2, "os", 2);
    duk_push_string(ctx, VM_IOTJS_ARCH);
    duk_put_prop_lstring(ctx, -2, "arch", 4);
    duk_push_string(ctx, VM_IOTJS_VERSION);
    duk_put_prop_lstring(ctx, -2, "version", 7);

    duk_put_prop_lstring(ctx, 0, "exports", 7);
    return 0;
}