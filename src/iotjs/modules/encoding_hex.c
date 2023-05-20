#include <iotjs/core/js.h>
#include <iotjs/core/module.h>

duk_ret_t native_iotjs_encoding_hex__encodeLen(duk_context *ctx)
{
    duk_double_t v = duk_require_number(ctx, 0);
    v *= 2;
    duk_pop(ctx);
    duk_push_number(ctx, v);
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__encodeToString(duk_context *ctx)
{
    duk_size_t sz_src;
    duk_uint8_t *src = duk_require_buffer_data(ctx, 0, &sz_src);
    if (sz_src > IOTJS_MAX_SAFE_INTEGER / 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "src length is too large");
        duk_throw(ctx);
    }
    duk_uint8_t *dst = duk_push_buffer(ctx, sz_src * 2, 0);
    duk_size_t j = 0;
    duk_uint8_t v;
    const char *hextable = IOTJS_HEX_TABLE;
    for (duk_size_t i = 0; i < sz_src; i++)
    {
        v = src[i];
        dst[j] = hextable[v >> 4];
        dst[j + 1] = hextable[v & 0x0f];
        j += 2;
    }
    duk_buffer_to_string(ctx, -1);
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__encode(duk_context *ctx)
{
    duk_size_t sz_dst, sz_src;
    duk_uint8_t *dst = duk_require_buffer(ctx, 0, &sz_dst);
    duk_uint8_t *src = duk_require_buffer(ctx, 1, &sz_src);
    if (sz_src > IOTJS_MAX_SAFE_INTEGER / 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "src length is too large");
        duk_throw(ctx);
    }
    duk_size_t j = 0;
    duk_uint8_t v;
    const char *hextable = IOTJS_HEX_TABLE;
    for (duk_size_t i = 0; i < sz_src; i++)
    {
        v = src[i];
        dst[j] = hextable[v >> 4];
        dst[j + 1] = hextable[v & 0x0f];
        j += 2;
    }
    duk_pop_2(ctx);
    duk_push_number(ctx, j);
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__decodedLen(duk_context *ctx)
{
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__decodeString(duk_context *ctx)
{
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__decode(duk_context *ctx)
{
    return 1;
}

duk_ret_t native_iotjs_encoding_hex_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__encodeLen, 1);
    duk_put_prop_lstring(ctx, -2, "encodeLen", 9);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__encodeToString, 1);
    duk_put_prop_lstring(ctx, -2, "encodeToString", 14);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__encode, 2);
    duk_put_prop_lstring(ctx, -2, "encode", 6);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__decodedLen, 1);
    duk_put_prop_lstring(ctx, -2, "decodedLen", 10);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__decodeString, 1);
    duk_put_prop_lstring(ctx, -2, "decodeString", 12);
    duk_push_c_function(ctx, native_iotjs_encoding_hex__decode, 2);
    duk_put_prop_lstring(ctx, -2, "decode", 6);
    return 0;
}