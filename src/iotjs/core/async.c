#include <iotjs/core/async.h>
#include <iotjs/core/defines.h>
#include <iotjs/core/js.h>
#include <iotjs/core/memory.h>
#include <stdio.h>
void vm_snapshot_create(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n, duk_bool_t move)
{
    duk_idx_t arr = duk_push_bare_array(ctx);
    if (n > arr)
    {
        duk_error(ctx, DUK_ERR_ERROR, "snapshot(%d), but top(%d)", n, arr);
    }
    duk_idx_t offset = arr - n;
    for (duk_size_t i = 0; i < n; i++)
    {
        duk_dup(ctx, offset + i);
        duk_put_prop_index(ctx, arr, i);
    }

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
    if (move)
    {
        duk_pop_n(ctx, n + 1);
    }
    else
    {
        duk_pop(ctx);
    }
}

duk_bool_t vm_snapshot_array(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot, duk_bool_t require)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (!duk_is_array(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }

    duk_swap_top(ctx, -2);
    if (del_snapshot)
    {
        duk_push_pointer(ctx, key);
        duk_del_prop(ctx, -2);
    }
    duk_pop(ctx);
    return 1;
}
duk_size_t vm_snapshot_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot, duk_bool_t require)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // ... bucket
    duk_push_pointer(ctx, key);
    duk_get_prop(ctx, -2);
    // ... bucket, snapshot
    if (!duk_is_array(ctx, -1))
    {
        duk_pop_2(ctx);
        if (require)
        {
            duk_error(ctx, DUK_ERR_ERROR, "snapshot not exists");
        }
        return 0;
    }
    duk_idx_t arr = duk_get_top_index(ctx);
    duk_size_t len = duk_get_length(ctx, arr);
    for (duk_size_t i = 2; i < len; i++)
    {
        duk_get_prop_index(ctx, arr, i);
    }
    switch (len)
    {
    case 0:
        break;
    case 1:
        duk_get_prop_index(ctx, arr, 0);
        duk_swap(ctx, -2, -3);
        duk_swap_top(ctx, -3);
        break;
    default:
        duk_get_prop_index(ctx, arr, 0);
        duk_get_prop_index(ctx, arr, 1);
        duk_swap(ctx, -1, arr);
        duk_swap(ctx, -2, arr - 1);
        break;
    }

    if (del_snapshot)
    {
        duk_push_pointer(ctx, key);
        duk_del_prop(ctx, -2);
    }
    duk_pop_2(ctx);
    return len;
}

void vm_snapshot_remove(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key)
{
    // ...
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_SNAPSHOTS);
    if (duk_is_undefined(ctx, -1))
    {
        duk_swap_top(ctx, -2);
        duk_pop(ctx);
        return;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // ... snapshots
    duk_get_prop_lstring(ctx, -1, bucket, sz_bucket);
    if (duk_is_undefined(ctx, -1))
    {
        duk_swap_top(ctx, -2);
        duk_pop(ctx);
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