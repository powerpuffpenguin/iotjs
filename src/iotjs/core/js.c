#include <iotjs/core/js.h>
#include <event2/event.h>
void vm_dump_context_stdout(duk_context *ctx)
{
    duk_push_context_dump(ctx);
    fprintf(stdout, "%s\n", duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
}
void *vm_get_finalizer_ptr(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "ptr");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    return duk_require_pointer(ctx, -1);
}
duk_ret_t _vm_context_finalizer(duk_context *ctx)
{
    vm_context_t *vm = (vm_context_t *)vm_get_finalizer_ptr(ctx);
    if (!vm)
    {
        return 0;
    }

    if (vm->eb)
    {
        event_base_free(vm->eb);
    }
    if (vm->threads)
    {
        thpool_wait(vm->threads);
        thpool_destroy(vm->threads);
    }
    pthread_mutex_destroy(&vm->mutex);
    IOTJS_FREE(vm);
    return 0;
}
void *vm_malloc_with_finalizer(duk_context *ctx, size_t sz, duk_c_function func)
{
    duk_require_stack(ctx, 3);
    // [...]
    duk_push_object(ctx);
    duk_push_lstring(ctx, "ptr", 3);
    duk_push_pointer(ctx, NULL);
    duk_put_prop(ctx, -3);

    duk_push_c_function(ctx, func, 1);
    duk_set_finalizer(ctx, -2);

    // [..., obj]
    duk_push_lstring(ctx, "ptr", 3);
    duk_push_pointer(ctx, NULL);
    duk_pop(ctx);

    // [..., obj, "ptr"]
    void *ptr = IOTJS_MALLOC(sz);
    if (!ptr)
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "vm_malloc_with_finalizer error");
        duk_throw(ctx);
    }
    memset(ptr, 0, sz);

    duk_push_pointer(ctx, ptr);
    duk_put_prop(ctx, -3);
    return ptr;
}

void _vm_init_context(duk_context *ctx)
{
    // [..., stash]
    duk_require_stack(ctx, 1 + 1 + 2);
    duk_get_prop_string(ctx, -1, "ctx");
    duk_context *main_ctx = (duk_context *)duk_require_pointer(ctx, -1);
    duk_pop(ctx);
    duk_del_prop_string(ctx, -1, "ctx");

    vm_context_t *vm = vm_malloc_with_finalizer(ctx, sizeof(vm_context_t), _vm_context_finalizer);
    // [..., stash, finalizer]
    duk_put_prop_string(ctx, -2, "context");

    if (pthread_mutex_init(&vm->mutex, NULL))
    {
        duk_pop(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "pthread_mutex_init error");
        duk_throw(ctx);
    }
    vm->ctx = main_ctx;
    vm->eb = event_base_new();
    if (!vm->eb)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_base_new error");
        duk_throw(ctx);
    }
    vm->threads = thpool_init(8);
    if (!vm->threads)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "thpool_init(8) error");
        duk_throw(ctx);
    }
}
vm_context_t *_vm_get_context(duk_context *ctx, BOOL completer)
{
    duk_require_stack(ctx, completer ? 4 : 3);
    duk_push_heap_stash(ctx);

    if (completer)
    {
        duk_get_prop_string(ctx, -1, "completer");
        duk_swap_top(ctx, -2);
    }

    duk_get_prop_string(ctx, -1, "context");
    duk_swap_top(ctx, -2);
    duk_pop(ctx); // [..., context]

    duk_get_prop_string(ctx, -1, "ptr");
    duk_swap_top(ctx, -2);
    duk_pop(ctx); // [..., ptr]

    vm_context_t *vm = duk_require_pointer(ctx, -1);
    duk_pop(ctx);
    return vm;
}
vm_context_t *vm_get_context(duk_context *ctx)
{
    return _vm_get_context(ctx, FALSE);
}

vm_async_job_t *vm_new_async_job(duk_context *ctx, vm_async_job_function work, size_t sz_arg)
{
    vm_context_t *vm = _vm_get_context(ctx, TRUE); // [..., completer]
    size_t sz_event = event_get_struct_event_size();
    vm_async_job_t *job = (vm_async_job_t *)IOTJS_MALLOC(sizeof(vm_async_job_t) + sz_event + sz_arg);
    if (!job)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc vm_async_job_t error");
        duk_throw(ctx);
    }
    job->vm = vm;
    job->ev = NULL;
    job->work = work;
    job->arg = NULL;
    if (!duk_check_stack(ctx, 1 + 2))
    {
        IOTJS_FREE(job);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "vm_new_async_job duk_check_stack error");
        duk_throw(ctx);
    }
    duk_push_object(ctx); // [..., completer, job]
    duk_push_pointer(ctx, job);
    duk_put_prop_string(ctx, -2, "ptr");
    // duk_set_finalizer(ctx,)

    return NULL;
}