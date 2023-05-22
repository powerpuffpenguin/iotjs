
#include <iotjs/core/js.h>
#include <iotjs/modules/xxd.h>
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

    duk_eval_lstring(ctx, (const char *)iotjs_modules_js_tsc_net_min_js, iotjs_modules_js_tsc_net_min_js_len);
    duk_swap_top(ctx, -2);

    // [ func, exports]
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    duk_call(ctx, 2);
    return 0;
}