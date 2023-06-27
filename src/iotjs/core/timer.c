#include <iotjs/core/timer.h>
#include <iotjs/core/configure.h>
#include <iotjs/core/js.h>
#include <iotjs/core/finalizer.h>
typedef struct
{
    vm_context_t *vm;
    event_t *ev;
} _vm_timer_t;

static void timer_free(void *arg)
{
#ifdef VM_TRACE_FINALIZER
    puts("timer_free");
#endif
    if (!arg)
    {
        return;
    }
    _vm_timer_t *timer = arg;
    if (timer->ev)
    {
        event_del(timer->ev);
    }
}
static void _vm_timer_handler(finalizer_t *finalizer, BOOL interval)
{
    _vm_timer_t *timer = finalizer->p;
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

    duk_push_pointer(ctx, finalizer);
    duk_get_prop(ctx, -2);
    if (!duk_is_object(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }                      // [..., timeout, timer]
    duk_swap_top(ctx, -2); // [..., timer, timeout]
    if (!interval)
    {
        duk_push_pointer(ctx, finalizer);
        duk_del_prop(ctx, -2);
    }
    duk_pop(ctx); // [..., timer]

    duk_get_prop_lstring(ctx, -1, "cb", 2);
    duk_call(ctx, 0);
    if (!interval)
    {
        duk_pop(ctx);
        vm_finalizer_free(ctx, -1, timer_free);
        duk_pop(ctx);
    }
    else
    {
        duk_pop_2(ctx);
    }
}
static void _vm_interval_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_handler((finalizer_t *)arg, TRUE);
}
static void _vm_timeout_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_handler((finalizer_t *)arg, FALSE);
}

static int _nativa_set_timer(duk_context *ctx, BOOL interval)
{
    if (!duk_is_callable(ctx, 0) || duk_is_null_or_undefined(ctx, 1))
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
    duk_pop(ctx);
    if (interval && ms < 0)
    {
        ms = 1;
    }

    // [func]
    vm_context_t *vm = vm_get_context(ctx);
    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(_vm_timer_t) + event_get_struct_event_size());
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "cb", 2); // [timer]
    _vm_timer_t *timer = finalizer->p;

    timer->vm = vm;
    timer->ev = NULL;
    // timer->ev = event_new(vm->eb, -1, interval ? EV_PERSIST : 0, interval ? _vm_interval_handler : _vm_timeout_handler, timer);
    event_t *ev = (event_t *)((char *)timer + sizeof(_vm_timer_t));
    if (event_assign(ev, vm->eb, -1, interval ? EV_PERSIST : 0, interval ? _vm_interval_handler : _vm_timeout_handler, finalizer))
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
    time_value_t tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    if (event_add(ev, &tv))
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
    finalizer->free = timer_free;
    timer->ev = ev;

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

    duk_push_pointer(ctx, finalizer);
    duk_swap_top(ctx, -2);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);

    duk_push_pointer(ctx, finalizer);
    return 1;
}
static duk_ret_t nativa_setTimeout(duk_context *ctx)
{
    duk_ret_t ret = _nativa_set_timer(ctx, FALSE);
    return ret;
}
static duk_ret_t nativa_setInterval(duk_context *ctx)
{
    return _nativa_set_timer(ctx, TRUE);
}
static void nativa_clear_timer(duk_context *ctx, BOOL interval)
{
    finalizer_t *finalizer = duk_require_pointer(ctx, 0);
    duk_pop(ctx);

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
    duk_pop(ctx); // [timeout]

    duk_push_pointer(ctx, finalizer);
    duk_get_prop(ctx, -2);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop_2(ctx);

        return;
    }
    // timeout, obj
    duk_swap_top(ctx, -2);
    duk_push_pointer(ctx, finalizer);
    duk_del_prop(ctx, -2);
    duk_pop(ctx);

    vm_finalizer_free(ctx, -1, timer_free);
    duk_pop(ctx);
}
static duk_ret_t nativa_clearTimeout(duk_context *ctx)
{
    if (duk_is_pointer(ctx, 0))
    {
        nativa_clear_timer(ctx, FALSE);
    }
    return 0;
}
static duk_ret_t nativa_clearInterval(duk_context *ctx)
{
    if (duk_is_pointer(ctx, 0))
    {
        nativa_clear_timer(ctx, TRUE);
    }
    return 0;
}
void _vm_init_timer(duk_context *ctx)
{
    duk_require_stack(ctx, 2);

    duk_push_c_lightfunc(ctx, nativa_setTimeout, 2, 2, 0);
    duk_put_prop_lstring(ctx, -2, "setTimeout", 10);
    duk_push_c_lightfunc(ctx, nativa_clearTimeout, 1, 1, 0);
    duk_put_prop_lstring(ctx, -2, "clearTimeout", 12);
    duk_push_c_lightfunc(ctx, nativa_setInterval, 2, 2, 0);
    duk_put_prop_lstring(ctx, -2, "setInterval", 11);
    duk_push_c_lightfunc(ctx, nativa_clearInterval, 1, 1, 0);
    duk_put_prop_lstring(ctx, -2, "clearInterval", 13);
}