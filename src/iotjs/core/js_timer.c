#include <iotjs/core/js_timer.h>
typedef struct
{
    vm_context_t *vm;
    event_t *ev;
} _vm_timer_t;

void _vm_timer_handler(_vm_timer_t *timer, BOOL interval)
{
    duk_context *ctx = timer->vm->ctx;
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, interval ? "interval" : "timeout");
    duk_swap_top(ctx, -2);
    duk_pop(ctx); // [..., timeout]

    duk_push_pointer(ctx, timer);
    duk_get_prop(ctx, -2);
    if (!duk_is_object(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }                      // [..., timeout, timer]
    duk_swap_top(ctx, -2); // [..., timer, timeout]
    if (!interval)
    {
        duk_push_pointer(ctx, timer);
        duk_del_prop(ctx, -2);
    }
    duk_pop(ctx); // [..., timer]

    duk_get_prop_string(ctx, -1, "cb");
    duk_call(ctx, 0);
    duk_pop_2(ctx);
}
void _vm_interval_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_handler((_vm_timer_t *)arg, TRUE);
}
void _vm_timeout_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_handler((_vm_timer_t *)arg, FALSE);
}

duk_ret_t _vm_iotjs_timer_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "ptr");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    _vm_timer_t *timer = (_vm_timer_t *)duk_require_pointer(ctx, -1);
    if (timer->ev)
    {
        event_free(timer->ev);
    }
    IOTJS_FREE(timer);
}
int _vm_iotjs_nativa_set_timer(duk_context *ctx, BOOL interval)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs < 1 || !duk_is_function(ctx, 0))
    {
        return 0;
    }
    duk_int_t ms = 0;
    if (duk_is_number(ctx, 1))
    {
        ms = duk_get_int(ctx, -1);
        if (ms < 0)
        {
            ms = 0;
        }
    }
    if (nargs > 1)
    {
        duk_pop_n(ctx, nargs - 1);
    }
    if (interval && ms < 0)
    {
        ms = 1;
    }

    // [func]
    vm_context_t *vm = vm_get_context(ctx);
    duk_push_object(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_string(ctx, -2, "cb"); // [timer]
    _vm_timer_t *timer = (_vm_timer_t *)IOTJS_MALLOC(sizeof(_vm_timer_t));
    if (!timer)
    {
        if (interval)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setInterval] malloc _vm_timer_t error");
        }
        else
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setTimeout] malloc _vm_timer_t error");
        }
        duk_throw(ctx);
    }
    timer->vm = vm;
    duk_push_pointer(ctx, timer);
    duk_put_prop_string(ctx, -2, "ptr");
    duk_push_c_function(ctx, _vm_iotjs_timer_finalizer, 1);
    duk_set_finalizer(ctx, -2);

    timer->ev = event_new(vm->eb, -1, interval ? EV_PERSIST : 0, interval ? _vm_interval_handler : _vm_timeout_handler, timer);
    if (!timer->ev)
    {
        duk_pop(ctx);
        if (interval)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setInterval] event_new error");
        }
        else
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setTimeout] event_new error");
        }
        duk_throw(ctx);
    }
    time_value_t tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    if (event_add(timer->ev, &tv))
    {
        duk_pop(ctx);
        if (interval)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setInterval] event_add error");
        }
        else
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setTimeout] event_add error");
        }
        duk_throw(ctx);
    }

    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, interval ? "interval" : "timeout");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -2); // [obj, timer]

    duk_push_pointer(ctx, timer);
    duk_swap_top(ctx, -2);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);

    duk_push_pointer(ctx, timer);
    return 1;
}
duk_ret_t _vm_iotjs_nativa_setTimeout(duk_context *ctx)
{
    return _vm_iotjs_nativa_set_timer(ctx, FALSE);
}
duk_ret_t _vm_iotjs_nativa_setInterval(duk_context *ctx)
{
    return _vm_iotjs_nativa_set_timer(ctx, TRUE);
}
void _vm_iotjs_nativa_clear_timer(duk_context *ctx, BOOL interval)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, interval ? "interval" : "timeout");
    duk_swap_top(ctx, -2);
    duk_pop(ctx); // [ptr, timeout]

    duk_swap_top(ctx, -2);
    duk_del_prop(ctx, -2);
}
duk_ret_t _vm_iotjs_nativa_clearTimeout(duk_context *ctx)
{
    if (duk_is_pointer(ctx, 0))
    {
        _vm_iotjs_nativa_clear_timer(ctx, FALSE);
    }
    return 0;
}
duk_ret_t _vm_iotjs_nativa_clearInterval(duk_context *ctx)
{
    if (duk_is_pointer(ctx, 0))
    {
        _vm_iotjs_nativa_clear_timer(ctx, TRUE);
    }
    return 0;
}
void _vm_init_timer(duk_context *ctx)
{
    duk_require_stack_top(ctx, duk_get_top(ctx) + 3);
    duk_push_heap_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "timeout");
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "interval");
    duk_pop(ctx);

    duk_push_c_function(ctx, _vm_iotjs_nativa_setTimeout, 2);
    duk_put_prop_string(ctx, -2, "setTimeout");
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearTimeout, 1);
    duk_put_prop_string(ctx, -2, "clearTimeout");
    duk_push_c_function(ctx, _vm_iotjs_nativa_setInterval, 2);
    duk_put_prop_string(ctx, -2, "setInterval");
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearInterval, 1);
    duk_put_prop_string(ctx, -2, "clearInterval");
}