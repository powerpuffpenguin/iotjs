#include <iotjs/core/async.h>
#include <iotjs/core/defines.h>
#include <iotjs/core/js.h>
#include <stdio.h>
static duk_context *vm_snapshot_impl(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n, duk_uint8_t copy)
{
    // duk_idx_t id = duk_push_thread(ctx);
    duk_push_thread(ctx);
    duk_context *snapshot = duk_require_context(ctx, -1);
    if (n)
    {
        if (copy)
        {
            duk_swap_top(ctx, -n - 1);
            duk_xcopy_top(snapshot, ctx, n);
            if (n > 1)
            {
                duk_insert(snapshot, 0);
            }
            duk_swap_top(ctx, -n - 1);
        }
        else
        {
            duk_swap_top(ctx, -n - 1);
            duk_xmove_top(snapshot, ctx, 1);
            if (n > 1)
            {
                duk_xmove_top(snapshot, ctx, n - 1);
            }
        }
    }
    // duk_push_number(ctx, id);
    // duk_put_prop_string(ctx, -2, "snapshot");

    // ... snapshot
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_lstring(ctx, -3, VM_STASH_KEY_SNAPSHOTS);
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // ... snapshot, snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_lstring(ctx, -3, bucket, sz_bucket);
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshot, bucket
    duk_swap_top(ctx, -2);
    duk_push_pointer(ctx, key);
    duk_swap_top(ctx, -2);
    duk_put_prop(ctx, -3);

    // bucket
    duk_pop(ctx);

    return snapshot;
}

duk_context *vm_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n)
{
    return vm_snapshot_impl(ctx, bucket, sz_bucket, key, n, 0);
}
duk_context *vm_snapshot_copy(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n)
{
    return vm_snapshot_impl(ctx, bucket, sz_bucket, key, n, 1);
}

duk_context *vm_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        return NULL;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        return NULL;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        return NULL;
    }
    duk_context *snapshot = duk_require_context(ctx, -1);
    duk_idx_t n = duk_get_top(snapshot);
    if (n > 0)
    {
        if (del_snapshot)
        {
            duk_xmove_top(ctx, snapshot, n);
        }
        else
        {
            duk_xcopy_top(ctx, snapshot, n);
        }
    }
    duk_remove(ctx, -n - 1);

    // bucket, v0, v1, ... vn-1
    if (del_snapshot)
    {
        duk_push_pointer(ctx, key);
        duk_del_prop(ctx, -n - 2);
        duk_remove(ctx, -n - 1);
        return NULL;
    }
    else
    {
        duk_remove(ctx, -n - 1);
        return snapshot;
    }
}
duk_context *vm_require_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
    }
    duk_context *snapshot = duk_require_context(ctx, -1);
    duk_pop_2(ctx);
    return snapshot;
}
void vm_remove_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // ... bucket
    duk_push_pointer(ctx, key);
    duk_del_prop(ctx, -2);
    duk_pop(ctx);
}

duk_context *vm_new_completer(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n)
{

    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // ... _iotjs
    duk_get_prop_lstring(ctx, -1, VM_IOTJS_KEY_COMPLETER);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... class Completer
    duk_new(ctx, 0);
    duk_get_prop_lstring(ctx, -1, "promise", 7);

    // ..., completer, promise
    if (n)
    {
        duk_insert(ctx, -n - 2);
    }
    else
    {
        duk_swap_top(ctx, -2);
    }

    // ... promise, completer
    duk_context *snapshot = vm_snapshot(ctx, bucket, sz_bucket, key, n + 1);
    return snapshot;
}
void vm_resolve(duk_context *ctx, duk_idx_t obj_idx)
{
    duk_push_lstring(ctx, "resolve", 7);
    duk_swap_top(ctx, -2);
    duk_call_prop(ctx, obj_idx < 0 ? obj_idx - 1 : obj_idx, 1);
}
void vm_reject(duk_context *ctx, duk_idx_t obj_idx)
{
    duk_push_lstring(ctx, "reject", 6);
    duk_swap_top(ctx, -2);
    duk_call_prop(ctx, obj_idx < 0 ? obj_idx - 1 : obj_idx, 1);
}
void vm_complete_lightfunc(duk_context *ctx, duk_c_function f, void *arg)
{
    duk_push_c_lightfunc(ctx, f, 1, 1, 0);
    duk_push_pointer(ctx, arg);
    duk_call(ctx, 1);
}
void vm_complete_lightfunc_noresult(duk_context *ctx, duk_c_function f, void *arg)
{
    duk_push_c_lightfunc(ctx, f, 1, 1, 0);
    duk_push_pointer(ctx, arg);
    duk_call(ctx, 1);
    duk_pop(ctx);
}

void vm_async_completer_args(duk_context *ctx, void *key)
{
    duk_require_stack(ctx, 3);
    duk_new(ctx, 0);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "args", 4);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_ASYNC);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
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
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // error, {}
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // error, {}, completer
    duk_push_pointer(ctx, key);
    duk_del_prop(ctx, -3);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
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
