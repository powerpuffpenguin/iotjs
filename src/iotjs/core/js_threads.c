#include <iotjs/core/js_threads.h>

duk_ret_t _vm_threads_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "ptr");
    threads_t *threads = (threads_t *)duk_get_pointer(ctx, -1);
    thpool_wait(threads);
    thpool_destroy(threads);
    return 0;
}
void _vm_init_threads(duk_context *ctx)
{
    duk_require_stack_top(ctx, duk_get_top(ctx) + 3);
    threads_t *threads = thpool_init(10);
    if (!threads)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "thpool_init error");
        duk_throw(ctx);
    }
    duk_push_object(ctx);
    duk_push_pointer(ctx, threads);
    duk_put_prop_string(ctx, -2, "ptr");
    duk_push_c_function(ctx, _vm_threads_finalizer, 1);
    duk_set_finalizer(ctx, -2);
    duk_put_prop_string(ctx, -2, "threads");
}
threads_t *vm_get_threads(duk_context *ctx)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "threads");
    if (!duk_is_object(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "stash.events invalid");
        duk_throw(ctx);
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, "ptr");
    threads_t *threads = (threads_t *)duk_get_pointer(ctx, -1);
    duk_pop_2(ctx);
    return threads;
}
BOOL vm_add_work(threads_t *threads, vm_thread_work_t work, void *arg)
{
    if (thpool_add_work(threads, work, arg))
    {
        return FALSE;
    }
    return TRUE;
}