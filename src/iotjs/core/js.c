#include <iotjs/core/js.h>
void vm_dump_context_stdout(duk_context *ctx)
{
    duk_push_context_dump(ctx);
    fprintf(stdout, "%s\n", duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
}
event_base_t *vm_event_base(duk_context *ctx)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "events");
    if (!duk_is_object(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events invalid");
        duk_throw(ctx);
    }
    if (!duk_get_prop_string(ctx, -1, "eb"))
    {
        duk_pop_3(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events.eb not exists");
        duk_throw(ctx);
    }
    if (!duk_is_pointer(ctx, -1))
    {
        duk_pop_3(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events.eb invalid");
        duk_throw(ctx);
    }
    event_base_t *eb = (event_base_t *)duk_get_pointer(ctx, -1);
    duk_pop_3(ctx);
    return eb;
}
duk_context *vm_context(duk_context *ctx)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "ctx");
    if (!duk_is_pointer(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.ctx invalid");
        duk_throw(ctx);
    }
    duk_context *c = duk_get_pointer(ctx, -1);
    duk_pop_2(ctx);
    return c;
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
    free(vm);
    return 0;
}
void vm_init_context(duk_context *ctx)
{
    duk_require_stack_top(ctx, duk_get_top(ctx) + 1 + 2);
    vm_context_t *vm = (vm_context_t *)malloc(sizeof(vm_context_t));
    if (!vm)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc vm_context_t error");
        duk_throw(ctx);
    }
    memset(vm, 0, sizeof(vm_context_t));
    duk_push_object(ctx);
    duk_push_pointer(ctx, vm);
    duk_put_prop_string(ctx, -2, "ptr");
    duk_push_c_function(ctx, _vm_context_finalizer, 1);
    duk_set_finalizer(ctx, -2);

    duk_put_prop_string(ctx, -2, "context");

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

    vm_dump_context_stdout(ctx);
    exit(1);
}