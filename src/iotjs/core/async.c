#include <iotjs/core/async.h>
#include <iotjs/core/defines.h>
void vm_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_lstring(ctx, -3, VM_STASH_KEY_SNAPSHOTS);
    }
    duk_remove(ctx, -2);
    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_lstring(ctx, -3, bucket, sz_bucket);
    }
    duk_remove(ctx, -2);
    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "stash snapshot not exists");
    }
}
void vm_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "stash snapshot not exists");
    }
    duk_remove(ctx, -2);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "stash snapshot not exists");
    }
    duk_remove(ctx, -2);
    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "stash snapshot not exists");
    }
}

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
void vm_async_complete(duk_context *ctx, void *key, duk_bool_t ok, duk_bool_t get)
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
    if (get)
    {
        duk_pop(ctx);
    }
    else
    {
        duk_pop_2(ctx);
    }
}
