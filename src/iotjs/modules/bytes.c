#include <duktape.h>

static duk_ret_t native_copy(duk_context *ctx)
{
    duk_size_t sz_dst;
    duk_uint8_t *dst = duk_require_buffer_data(ctx, 0, &sz_dst);
    duk_size_t sz_src;
    const duk_uint8_t *src;
    if (duk_is_string(ctx, 1))
    {
        src = duk_require_lstring(ctx, 1, &sz_src);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 1, &sz_src);
    }
    duk_size_t n = sz_dst < sz_src ? sz_dst : sz_src;
    memmove(dst, src, n);
    duk_pop_2(ctx);
    duk_push_number(ctx, n);
    return 1;
}
static duk_ret_t native_compare(duk_context *ctx)
{
    duk_size_t sz_dst;
    const unsigned char *dst;
    if (duk_is_string(ctx, 0))
    {
        dst = duk_require_lstring(ctx, 0, &sz_dst);
    }
    else
    {
        dst = duk_require_buffer_data(ctx, 0, &sz_dst);
    }
    duk_size_t sz_src;
    const unsigned char *src;
    if (duk_is_string(ctx, 1))
    {
        src = duk_require_lstring(ctx, 1, &sz_src);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 1, &sz_src);
    }
    duk_bool_t icase = 1;
    switch (duk_get_type(ctx, 2))
    {
    case DUK_TYPE_BOOLEAN:
        icase = duk_get_boolean(ctx, 2);
        break;
    case DUK_TYPE_NUMBER:
        if (duk_is_nan(ctx, 2) || !duk_get_number(ctx, 2))
        {
            icase = 0;
        }
        break;
    case DUK_TYPE_STRING:
    {
        duk_size_t sz;
        duk_require_lstring(ctx, 2, &sz);
        if (sz == 0)
        {
            icase = 0;
        }
    }
    break;
    case DUK_TYPE_NULL:
        icase = 0;
        break;
    case DUK_TYPE_UNDEFINED:
        icase = 0;
        break;
    }

    duk_size_t n = sz_dst < sz_src ? sz_dst : sz_src;
    unsigned char l, r;
    if (icase)
    {
        for (duk_size_t i = 0; i < n; i++)
        {
            l = dst[i];
            r = src[i];

            if (l >= 'a' && l <= 'z')
            {
                l += 'A' - 'a';
            }
            if (r >= 'a' && r <= 'z')
            {
                r += 'A' - 'a';
            }
            if (l == r)
            {
                continue;
            }
            duk_pop_2(ctx);
            duk_push_int(ctx, l < r ? -1 : 1);
            return 1;
        }
    }
    else
    {
        for (duk_size_t i = 0; i < n; i++)
        {
            l = dst[i];
            r = src[i];
            if (l == r)
            {
                continue;
            }
            duk_pop_2(ctx);
            duk_push_int(ctx, l < r ? -1 : 1);
            return 1;
        }
    }
    duk_pop_2(ctx);
    duk_push_int(ctx, 0);
    return 1;
}
static duk_ret_t native_set(duk_context *ctx)
{
    duk_size_t sz_dst;
    duk_uint8_t *dst = duk_require_buffer_data(ctx, 0, &sz_dst);
    int c = duk_require_number(ctx, 1);
    if (sz_dst)
    {
        memset(dst, c, sz_dst);
    }
    return 0;
}
duk_ret_t native_iotjs_bytes_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_push_c_lightfunc(ctx, native_copy, 2, 2, 0);
    duk_put_prop_lstring(ctx, -2, "copy", 4);
    duk_push_c_lightfunc(ctx, native_compare, 3, 3, 0);
    duk_put_prop_lstring(ctx, -2, "compare", 8);
    duk_push_c_lightfunc(ctx, native_set, 2, 2, 0);
    duk_put_prop_lstring(ctx, -2, "set", 3);
    return 0;
}