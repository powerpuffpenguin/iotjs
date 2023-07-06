#include <iotjs/core/async.h>
#include <iotjs/core/defines.h>
#include <iotjs/core/js.h>
#include <iotjs/core/memory.h>
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
vm_job_t *vm_new_job(duk_context *ctx, size_t in, size_t out)
{
    vm_context_t *vm = vm_get_context(ctx);
    size_t n = sizeof(vm_job_t) + in + out;
    duk_uint8_t *p = vm_malloc(n);
    if (!p)
    {
        duk_error(ctx, DUK_ERR_ERROR, "malloc job fail");
    }
    memset(p, 0, n);
    vm_job_t *job = (vm_job_t *)p;
    job->vm = vm;
    job->in = in ? (p + sizeof(vm_job_t)) : 0;
    job->out = out ? (p + sizeof(vm_job_t) + in) : 0;
    return job;
}

duk_bool_t vm_run_job(vm_job_t *job, void(work)(vm_job_t *job))
{
    return thpool_add_work(job->vm->threads, (void (*)(void *))(work), job) ? 0 : 1;
}
void vm_must_run_job(duk_context *ctx, vm_job_t *job, void (*work)(vm_job_t *job))
{
    if (thpool_add_work(job->vm->threads, (void (*)(void *))(work), job))
    {
        vm_free(job);
        duk_error(ctx, DUK_ERR_ERROR, "thpool_add_work fail");
    }
}