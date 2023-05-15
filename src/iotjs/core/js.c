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