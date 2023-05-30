#include <iotjs/core/async.h>
#include <iotjs/core/defines.h>
void vm_async_completer_args(duk_context *ctx, void *key)
{
    duk_require_stack(ctx, 3);
    duk_new(ctx, 0);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "args", 4);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_ASYNC);
    duk_remove(ctx, -2);
    duk_swap_top(ctx, -2);

    // ... {}, completer
    duk_get_prop_lstring(ctx, -1, "promise", 7);
    duk_insert(ctx, -3);

    // ... promise, {}, completer
    duk_push_pointer(ctx, key);
    duk_swap_top(ctx, -2);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);
}
void vm_async_complete(duk_context *ctx, void *key, duk_bool_t ok)
{
    duk_require_stack(ctx, 3);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_ASYNC);
    duk_remove(ctx, -2);

    // error, {}
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // error, {}, completer
    duk_push_pointer(ctx, key);
    duk_del_prop(ctx, -3);
    duk_remove(ctx, -2);
    // error, completer
    duk_swap_top(ctx, -2);
    if (ok)
    {
        duk_push_lstring(ctx, "resolve", 7);
    }
    else
    {
        duk_push_lstring(ctx, "reject", 6);
    }
    duk_swap_top(ctx, -2);
    duk_call_prop(ctx, -3, 1);
    duk_pop_2(ctx);
}
