#include <iotjs/core/js_timer.h>
typedef struct
{
    struct event *ev;
    duk_context *ctx;
    uint8_t interval;
} _vm_timer_t;
void _vm_timer_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_t *timer = (_vm_timer_t *)arg;
    duk_context *ctx = timer->ctx;
    // timer
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_get_prop(ctx, -2);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    if (!duk_is_object(ctx, -1))
    {
        duk_pop(ctx);
        return;
    }
    // cb
    duk_get_prop_string(ctx, -1, "cb");
    if (!duk_is_function(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }
    if (duk_pcall(ctx, 0))
    {
        puts(duk_safe_to_string(ctx, -1));
        abort();
    }
    duk_pop_2(ctx);
    if (timer->interval)
    {
        return;
    }
    // remove
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_del_prop(ctx, -2);
}
duk_ret_t _vm_timer_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "timer");
    _vm_timer_t *timer = (_vm_timer_t *)duk_get_pointer(ctx, -1);
    if (timer->ev)
    {
        event_del(timer->ev);
        event_free(timer->ev);
    }
    free(timer);
    return 0;
}
_vm_timer_t *_vm_new_timer(duk_context *ctx, duk_int_t ms, BOOL interval)
{
    // timer
    struct event_base *eb = vm_event_base(ctx);
    duk_context *cm = vm_context(ctx);
    _vm_timer_t *timer = (_vm_timer_t *)malloc(sizeof(_vm_timer_t));
    if (!timer)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc timer error");
        duk_throw(ctx);
    }
    memset(timer, 0, sizeof(_vm_timer_t));
    timer->ctx = cm;
    if (interval)
    {
        timer->interval = TRUE;
    }
    duk_push_object(ctx);
    duk_push_pointer(ctx, timer);
    duk_put_prop_string(ctx, -2, "timer");
    duk_push_c_function(ctx, _vm_timer_finalizer, 1);
    duk_set_finalizer(ctx, -2);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "cb");

    // event
    timer->ev = event_new(eb, -1, interval ? (EV_TIMEOUT | EV_PERSIST) : EV_TIMEOUT, _vm_timer_handler, timer);
    if (!timer->ev)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_new timer error");
        duk_throw(ctx);
    }
    struct timeval tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    if (interval && ms == 0)
    {
        tv.tv_usec = 1;
    }
    if (event_add(timer->ev, &tv))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_add timer error");
        duk_throw(ctx);
    }
    return timer;
}
void _vm_set_timer(duk_context *ctx, _vm_timer_t *timer)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    if (!duk_is_object(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, -3, "timer");
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_dup(ctx, 0);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_safe_to_string(ctx, -1);
}
void _vm_iotjs_nativa_set_timer(duk_context *ctx, BOOL interval)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs < 1 || !duk_is_function(ctx, 0))
    {
        return;
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
    _vm_timer_t *timer = _vm_new_timer(ctx, ms, interval);
    duk_swap_top(ctx, 0);
    duk_pop(ctx);
    _vm_set_timer(ctx, timer);
}
duk_ret_t _vm_iotjs_nativa_setTimeout(duk_context *ctx)
{
    _vm_iotjs_nativa_set_timer(ctx, FALSE);
    return 1;
}
duk_ret_t _vm_iotjs_nativa_setInterval(duk_context *ctx)
{
    _vm_iotjs_nativa_set_timer(ctx, TRUE);
    return 1;
}
void _vm_iotjs_nativa_clear_timer(duk_context *ctx, BOOL interval)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    if (!duk_is_object(ctx, -1))
    {
        return;
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -2);
    duk_get_prop(ctx, -2);
    if (!duk_is_object(ctx, -1))
    {
        return;
    }
    // [stash.tiemr, timer]
    duk_get_prop_string(ctx, -1, "timer");
    _vm_timer_t *timer = (_vm_timer_t *)duk_get_pointer(ctx, -1);
    if (timer->interval != interval)
    {
        return;
    }
    event_del(timer->ev);
    event_free(timer->ev);
    timer->ev = NULL;

    duk_swap_top(ctx, -1);
    duk_pop(ctx);
    duk_safe_to_string(ctx, -1);
    duk_del_prop(ctx, -2);
}
duk_ret_t _vm_iotjs_nativa_clearTimeout(duk_context *ctx)
{
    if (duk_is_string(ctx, 0))
    {
        _vm_iotjs_nativa_clear_timer(ctx, FALSE);
    }
    return 0;
}
duk_ret_t _vm_iotjs_nativa_clearInterval(duk_context *ctx)
{
    if (duk_is_string(ctx, 0))
    {
        _vm_iotjs_nativa_clear_timer(ctx, TRUE);
    }
    return 0;
}
void _vm_init_timer(duk_context *ctx)
{
    duk_push_c_function(ctx, _vm_iotjs_nativa_setTimeout, 2);
    duk_put_prop_string(ctx, -2, "setTimeout");
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearTimeout, 1);
    duk_put_prop_string(ctx, -2, "clearTimeout");
    duk_push_c_function(ctx, _vm_iotjs_nativa_setInterval, 2);
    duk_put_prop_string(ctx, -2, "setInterval");
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearInterval, 1);
    duk_put_prop_string(ctx, -2, "clearInterval");
}