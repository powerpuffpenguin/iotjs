#include <iotjs/core/js.h>
#include <event2/event.h>
void vm_dump_context_stdout(duk_context *ctx)
{
    duk_push_context_dump(ctx);
    fprintf(stdout, "%s\n", duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
}

duk_ret_t _vm_context_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "ptr");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    vm_context_t *vm = (vm_context_t *)duk_get_pointer(ctx, -1);
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
void _vm_init_context(duk_context *ctx)
{
    duk_require_stack_top(ctx, duk_get_top(ctx) + 1 + 2);
    duk_get_prop_string(ctx, -1, "ctx");
    duk_context *main_ctx = (duk_context *)duk_require_pointer(ctx, -1);
    duk_pop(ctx);
    duk_del_prop_string(ctx, -1, "ctx");

    vm_context_t *vm = (vm_context_t *)IOTJS_MALLOC(sizeof(vm_context_t));
    if (!vm)
    {
        duk_pop(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc vm_context_t error");
        duk_throw(ctx);
    }
    memset(vm, 0, sizeof(vm_context_t));
    if (pthread_mutex_init(&vm->mutex, NULL))
    {
        IOTJS_FREE(vm);
        duk_pop(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "pthread_mutex_init error");
        duk_throw(ctx);
    }

    duk_push_object(ctx);
    duk_push_pointer(ctx, vm);
    duk_put_prop_string(ctx, -2, "ptr");
    duk_push_c_function(ctx, _vm_context_finalizer, 1);
    duk_set_finalizer(ctx, -2);

    duk_put_prop_string(ctx, -2, "context");
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
vm_context_t *vm_get_context(duk_context *ctx)
{
    duk_require_stack_top(ctx, duk_get_top(ctx) + 1 + 2);
    duk_push_heap_stash(ctx);

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