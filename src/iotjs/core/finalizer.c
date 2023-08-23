#include <iotjs/core/finalizer.h>
#include <iotjs/core/memory.h>
static duk_ret_t native_finalizer(duk_context *ctx)
{
    duk_get_prop_lstring(ctx, 0, "_p", 2);
    if (!duk_is_pointer(ctx, -1))
    {
        return 0;
    }
    finalizer_t *finalizer = duk_require_pointer(ctx, -1);
    if (finalizer->free)
    {
        finalizer->free(finalizer->p);
    }
    vm_free(finalizer);
    return 0;
}
static duk_ret_t native_finalizer_n(duk_context *ctx)
{
    duk_get_prop_lstring(ctx, 0, "_p", 2);
    if (!duk_is_pointer(ctx, -1))
    {
        return 0;
    }
    finalizer_t *finalizer = duk_require_pointer(ctx, -1);
    if (finalizer->free)
    {
        finalizer->free(finalizer->p);
    }
    vm_free(finalizer);
    return 0;
}
static duk_ret_t native_create_finalizer(duk_context *ctx)
{
    duk_require_stack_top(ctx, 3);
    finalizer_t **pp = duk_get_pointer(ctx, 0);
    duk_pop(ctx);
    finalizer_t *finalizer = vm_malloc(sizeof(finalizer_t));
    if (!finalizer)
    {
        duk_error(ctx, DUK_ERR_ERROR, "cannot  malloc finalizer");
    }
    *pp = finalizer;

    duk_require_stack(ctx, 3);
    duk_push_object(ctx);
    duk_push_pointer(ctx, finalizer);
    duk_put_prop_lstring(ctx, -2, "_p", 2);
    duk_push_c_lightfunc(ctx, native_finalizer, 1, 1, 0);
    duk_set_finalizer(ctx, -2);
    return 1;
}
static duk_ret_t native_create_finalizer_n(duk_context *ctx)
{
    duk_require_stack_top(ctx, 3);
    finalizer_t **pp = duk_get_pointer(ctx, 0);
    duk_size_t n = duk_require_number(ctx, 1);
    duk_pop_2(ctx);
    finalizer_t *finalizer = vm_malloc(sizeof(finalizer_t) + n);
    if (!finalizer)
    {
        duk_error(ctx, DUK_ERR_ERROR, "cannot  malloc finalizer");
    }
    *pp = finalizer;
    duk_require_stack(ctx, 3);
    duk_push_object(ctx);
    duk_push_pointer(ctx, finalizer);
    duk_put_prop_lstring(ctx, -2, "_p", 2);
    duk_push_c_lightfunc(ctx, native_finalizer_n, 1, 1, 0);
    duk_set_finalizer(ctx, -2);
    return 1;
}
finalizer_t *vm_create_finalizer(duk_context *ctx)
{
    finalizer_t *finalizer = NULL;
    duk_require_stack(ctx, 2);
    duk_push_c_lightfunc(ctx, native_create_finalizer, 1, 1, 0);
    duk_push_pointer(ctx, &finalizer);
    if (duk_pcall(ctx, 1))
    {
        if (finalizer)
        {
            vm_free(finalizer);
        }
        duk_throw(ctx);
    }
    finalizer->p = NULL;
    finalizer->free = NULL;
    return finalizer;
}
finalizer_t *vm_create_finalizer_n(duk_context *ctx, size_t n)
{
    finalizer_t *finalizer = NULL;
    duk_require_stack(ctx, 3);
    duk_push_c_lightfunc(ctx, native_create_finalizer_n, 2, 2, 0);
    duk_push_pointer(ctx, &finalizer);
    duk_push_uint(ctx, n);
    if (duk_pcall(ctx, 2))
    {
        if (finalizer)
        {
            vm_free(finalizer);
        }
        duk_throw(ctx);
    }
    finalizer->free = NULL;
    if (n)
    {
        finalizer->p = ((char *)finalizer) + sizeof(finalizer_t);
        memset(finalizer->p, 0, n);
    }
    else
    {
        finalizer->p = NULL;
    }
    return finalizer;
}
finalizer_t *vm_require_finalizer(duk_context *ctx, duk_idx_t idx, void (*freef)(void *p))
{
    duk_get_finalizer(ctx, idx);
    if (duk_is_undefined(ctx, -1))
    {
        duk_pop(ctx);
        duk_type_error(ctx, "finalizer not exist");
    }
    if (!duk_is_lightfunc(ctx, -1))
    {
        duk_type_error(ctx, "finalizer is inconsistent");
    }
    duk_pop(ctx);

    duk_get_prop_lstring(ctx, idx, "_p", 2);
    finalizer_t *finalizer = duk_require_pointer(ctx, -1);
    duk_pop(ctx);
    if (!finalizer || finalizer->free != freef)
    {
        duk_type_error(ctx, "finalizer is inconsistent");
    }

    return finalizer;
}
void *vm_finalizer_free(duk_context *ctx, duk_idx_t idx, void (*freef)(void *p))
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, idx, freef);
    void *p = finalizer->p;
    duk_del_prop_lstring(ctx, -1, "_p", 2);
    if (finalizer->free)
    {
        finalizer->free(finalizer->p);
    }
    vm_free(finalizer);
    return p;
}
