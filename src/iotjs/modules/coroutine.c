
#include <iotjs/core/defines.h>
#include <duktape.h>
#include <iotjs/modules/js/coroutine.h>
duk_ret_t native_iotjs_coroutine_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_coroutine_min_js, js_iotjs_modules_js_coroutine_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    duk_call(ctx, 2);
    return 0;
}