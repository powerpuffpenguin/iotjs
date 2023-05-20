#include <iotjs/modules/module.h>
#include <iotjs/core/js.h>
#include <iotjs/core/defines.h>
duk_ret_t native_iotjs_init(duk_context *ctx)
{
    duk_swap_top(ctx, 0);
    duk_pop_2(ctx);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_IOTJS);

    duk_put_prop_lstring(ctx, 0, "exports", 7);
    return 0;
}