#include <iotjs/core/js.h>
#include <event2/event.h>
#include <event2/util.h>
#include <netinet/in.h>
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
    if (vm->esb)
    {
        evdns_base_free(vm->esb, 1);
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
duk_ret_t vm_native_default_finalizer(duk_context *ctx)
{
    duk_get_prop_lstring(ctx, -1, "ptr", 3);
    void *p = duk_require_pointer(ctx, -1);
    if (p)
    {
        IOTJS_FREE(p);
    }
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

    duk_push_c_function(ctx, func ? func : vm_native_default_finalizer, 1);
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
void *vm_malloc_with_finalizer_init(duk_context *ctx, size_t sz, duk_c_function func, void (*init)(void *))
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
    init(ptr);

    duk_push_pointer(ctx, ptr);
    duk_put_prop(ctx, -3);
    return ptr;
}
void _vm_init_context(duk_context *ctx, duk_context *main)
{
    // [..., stash]
    duk_require_stack(ctx, 1 + 1 + 2);

    vm_context_t *vm = vm_malloc_with_finalizer(ctx, sizeof(vm_context_t), _vm_context_finalizer);
    // [..., stash, finalizer]
    duk_put_prop_string(ctx, -2, "context");

    if (pthread_mutex_init(&vm->mutex, NULL))
    {
        duk_pop(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "pthread_mutex_init error");
        duk_throw(ctx);
    }
    vm->ctx = main;
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
    duk_require_stack(ctx, completer ? 5 : 3);
    duk_push_heap_stash(ctx);

    if (completer)
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
        duk_get_prop_lstring(ctx, -1, VM_IOTJS_KEY_COMPLETER);
        duk_swap_top(ctx, -2);
        duk_pop(ctx);
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
vm_context_t *vm_get_context_flags(duk_context *ctx, duk_uint32_t flags)
{
    vm_context_t *vm = _vm_get_context(ctx, flags & VM_CONTEXT_FLAGS_COMPLETER ? TRUE : FALSE);
    if ((flags & VM_CONTEXT_FLAGS_ESB) && !vm->esb)
    {
        duk_push_heap_stash(ctx);
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_NAMESERVER);
        if (duk_is_string(ctx, -1))
        {
            vm->esb = evdns_base_new(vm->eb, EVDNS_BASE_DISABLE_WHEN_INACTIVE);
            if (!vm->esb)
            {
                duk_pop_2(ctx);
                duk_push_lstring(ctx, "evdns_base_new error", 20);
                duk_throw(ctx);
            }
            const char *nameserver = duk_get_string(ctx, -1);
            int err = evdns_base_nameserver_ip_add(vm->esb, nameserver);
            duk_pop_2(ctx);
            if (err)
            {
                duk_push_lstring(ctx, "evdns_base_nameserver_ip_add error: ", 37);
                duk_push_string(ctx, nameserver);
                duk_concat(ctx, 2);
                duk_throw(ctx);
            }
        }
        else
        {
            duk_pop_2(ctx);
            vm->esb = evdns_base_new(vm->eb, EVDNS_BASE_INITIALIZE_NAMESERVERS | EVDNS_BASE_DISABLE_WHEN_INACTIVE);
            if (!vm->esb)
            {
                duk_push_lstring(ctx, "evdns_base_new error", 20);
                duk_throw(ctx);
            }
        }
    }
    return vm;
}
duk_ret_t _vm_async_job_finalizer(duk_context *ctx)
{
    vm_async_job_t *job = vm_get_finalizer_ptr(ctx);
    if (!job)
    {
        return 0;
    }
    if (job->ev)
    {
        event_del(job->ev);
    }
    IOTJS_FREE(job);
    return 0;
}
void _vm_async_job_handler(evutil_socket_t fd, short events, void *arg)
{
    vm_async_job_t *job = (vm_async_job_t *)arg;

    duk_context *ctx = job->vm->ctx;
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_JOBS);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // [..., jobs]
    duk_push_pointer(ctx, job);
    duk_get_prop(ctx, -2);
    if (!duk_is_object(ctx, -1))
    {
        // 因爲其它某些原因，異步已經取消或被關閉(目前版本這段代碼應該永遠不會執行到)
        duk_pop_2(ctx);
        return;
    }
    duk_push_pointer(ctx, job);
    duk_del_prop(ctx, -3);

    if (!job->complete)
    {
        // 異步完成後不需要通知
        duk_pop_2(ctx);
        return;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // [..., job]
    duk_dup_top(ctx);
    duk_push_c_function(ctx, job->complete, 1);
    duk_swap_top(ctx, -2);
    if (!duk_pcall(ctx, 1))
    {
        duk_pop(ctx);
        return;
    }

    // 回調出現錯誤，嘗試通知錯誤
    vm_reject_async_job(ctx, -2);
}
void vm_complete_async_job(vm_async_job_t *job)
{
    event_active(job->ev, 0, 0);
}
void _vm_async_job_work(void *arg)
{
    vm_async_job_t *job = (vm_async_job_t *)arg;
    job->work(job, job->in);
}
vm_async_job_t *vm_new_async_job(duk_context *ctx, vm_async_job_function work, size_t sz_in, size_t sz_out)
{
    vm_context_t *vm = _vm_get_context(ctx, TRUE); // [..., completer]
    size_t sz_event = event_get_struct_event_size();
    char *p = vm_malloc_with_finalizer(ctx, sizeof(vm_async_job_t) + sz_event + sz_in + sz_out, _vm_async_job_finalizer);
    if (!p)
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc(vm_async_job_t) error");
        duk_throw(ctx);
    }
    vm_async_job_t *job = (vm_async_job_t *)p;
    job->vm = vm;
    job->work = work;
    p += sizeof(vm_async_job_t);

    event_t *ev = (event_t *)(p);
    p += sz_event;

    if (event_assign(ev, vm->eb, -1, 0, _vm_async_job_handler, job))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_assign(vm_async_job_t) error");
        duk_throw(ctx);
    }
    if (event_add(ev, NULL))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_add(vm_async_job_t) error");
        duk_throw(ctx);
    }
    job->ev = ev;

    duk_swap_top(ctx, -2);
    duk_new(ctx, 0);
    duk_put_prop_lstring(ctx, -2, VM_IOTJS_KEY_COMPLETER);

    if (sz_in)
    {
        job->in = p;
        p += sz_in;
    }
    if (sz_out)
    {
        job->out = p;
    }
    return job;
}
void vm_execute_async_job(duk_context *ctx, vm_async_job_t *job)
{
    duk_require_stack(ctx, 1 + 1 + 1 + 2);
    // [..., job]
    if (thpool_add_work(job->vm->threads, _vm_async_job_work, job))
    {
        duk_pop(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "thpool_add_work error");
        duk_throw(ctx);
    }

    duk_get_prop_lstring(ctx, -1, VM_IOTJS_KEY_COMPLETER);
    duk_get_prop_lstring(ctx, -1, "promise", 7);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -2);

    // [..., promise, job]
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_JOBS);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -2);

    // [..., promise, jobs, job]
    duk_push_pointer(ctx, job);
    duk_swap_top(ctx, -2);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);
}

vm_async_job_t *vm_get_async_job(duk_context *ctx)
{
    duk_require_stack(ctx, 2);
    duk_get_prop_lstring(ctx, -1, "ptr", 3);
    vm_async_job_t *job = (vm_async_job_t *)duk_require_pointer(ctx, -1);
    duk_pop(ctx);
    return job;
}
void vm_reject_async_job(duk_context *ctx, duk_idx_t i)
{
    duk_require_stack(ctx, 3);
    duk_get_prop_lstring(ctx, i, VM_IOTJS_KEY_COMPLETER);
    duk_swap_top(ctx, -2);
    duk_push_lstring(ctx, "reject", 6);
    duk_swap_top(ctx, -2);
    duk_call_prop(ctx, -3, 1);
}
void vm_resolve_async_job(duk_context *ctx, duk_idx_t i)
{
    duk_require_stack(ctx, 3);
    duk_get_prop_lstring(ctx, i, VM_IOTJS_KEY_COMPLETER);
    duk_swap_top(ctx, -2);
    duk_push_lstring(ctx, "resolve", 7);
    duk_swap_top(ctx, -2);
    duk_call_prop(ctx, -3, 1);
}
void vm_require_date(duk_context *ctx)
{
    duk_require_stack(ctx, 3);
    duk_push_global_object(ctx);
    duk_get_prop_lstring(ctx, -1, "Date", 4);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
}
void vm_push_error_object(duk_context *ctx, int code, const char *message)
{
    duk_push_object(ctx);
    duk_push_int(ctx, code);
    duk_put_prop_lstring(ctx, -2, "code", 4);
    duk_push_string(ctx, message);
    duk_put_prop_lstring(ctx, -2, "message", 7);
}