#include <iotjs/core/timer.h>
#include <iotjs/core/js.h>
typedef struct
{
    vm_context_t *vm;
    event_t *ev;
} _vm_timer_t;

void _vm_timer_handler(_vm_timer_t *timer, BOOL interval)
{
    duk_context *ctx = timer->vm->ctx;
    duk_push_heap_stash(ctx);
    if (interval)
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_INTERVAL);
    }
    else
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_TIMEOUT);
    }
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

    duk_get_prop_lstring(ctx, -1, "cb", 2);
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
    _vm_timer_t *timer = (_vm_timer_t *)vm_get_finalizer_ptr(ctx);
    if (!timer)
    {
        return 0;
    }

    if (timer->ev)
    {
        event_del(timer->ev);
        // event_free(timer->ev);
    }
    IOTJS_FREE(timer);
    return 0;
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
    _vm_timer_t *timer = vm_malloc_with_finalizer(ctx, sizeof(_vm_timer_t) + event_get_struct_event_size(), _vm_iotjs_timer_finalizer);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "cb", 2); // [timer]

    timer->vm = vm;
    timer->ev = NULL;
    // timer->ev = event_new(vm->eb, -1, interval ? EV_PERSIST : 0, interval ? _vm_interval_handler : _vm_timeout_handler, timer);
    event_t *ev = (event_t *)((char *)timer + sizeof(_vm_timer_t));
    if (event_assign(ev, vm->eb, -1, interval ? EV_PERSIST : 0, interval ? _vm_interval_handler : _vm_timeout_handler, timer))
    {
        duk_pop(ctx);
        if (interval)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setInterval] event_assign error");
        }
        else
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "[setTimeout] event_assign error");
        }
        duk_throw(ctx);
    }
    timer->ev = ev;
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
    if (interval)
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_INTERVAL);
    }
    else
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_TIMEOUT);
    }
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
    if (interval)
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_INTERVAL);
    }
    else
    {
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_TIMEOUT);
    }
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
    duk_require_stack(ctx, 2);

    duk_push_c_function(ctx, _vm_iotjs_nativa_setTimeout, 2);
    duk_put_prop_lstring(ctx, -2, "setTimeout", 10);
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearTimeout, 1);
    duk_put_prop_lstring(ctx, -2, "clearTimeout", 12);
    duk_push_c_function(ctx, _vm_iotjs_nativa_setInterval, 2);
    duk_put_prop_lstring(ctx, -2, "setInterval", 11);
    duk_push_c_function(ctx, _vm_iotjs_nativa_clearInterval, 1);
    duk_put_prop_lstring(ctx, -2, "clearInterval", 13);
}