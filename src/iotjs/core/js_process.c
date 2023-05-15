#include <iotjs/core/js_process.h>
void _vm_init_process(duk_context *ctx)
{
    duk_push_object(ctx);
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "argv");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_put_prop_string(ctx, -2, "argv");
    duk_put_prop_string(ctx, -2, "process");
}