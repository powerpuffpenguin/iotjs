#include <iotjs/core/js.h>
#include <iotjs/core/module.h>

duk_ret_t native_iotjs_encoding_hex__encodeLen(duk_context *ctx)
{
    duk_uint_t v = duk_require_uint(ctx, 0);
    v *= 2;
    duk_pop(ctx);
    duk_push_uint(ctx, v);
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__encodeToString(duk_context *ctx)
{
    duk_size_t sz_src;
    duk_uint8_t *src = duk_require_buffer_data(ctx, 0, &sz_src);
    if (!sz_src)
    {
        duk_push_lstring(ctx, "", 0);
        return 1;
    }
    else if (sz_src > IOTJS_MAX_SAFE_INTEGER / 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
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
    duk_uint8_t *dst = duk_require_buffer_data(ctx, 0, &sz_dst);
    duk_uint8_t *src = duk_require_buffer_data(ctx, 1, &sz_src);
    if (!sz_src)
    {
        duk_push_uint(ctx, 0);
        return 1;
    }
    else if (sz_src > IOTJS_MAX_SAFE_INTEGER / 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
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
    duk_uint_t v = duk_require_uint(ctx, 0);
    v /= 2;
    duk_pop(ctx);
    duk_push_uint(ctx, v);
    return 1;
}
void native_iotjs_encoding_hex__decode_buffer(duk_context *ctx, duk_uint8_t *dst, duk_uint8_t *src, duk_size_t sz_src)
{
    duk_size_t i = 0, j = 1;
    duk_uint8_t p, q, a, b;
    IOTJS_HEX_REVERSS_TABLE(reverseHexTable);
    for (; j < sz_src; j += 2)
    {
        p = src[j - 1];
        a = reverseHexTable[p];
        if (a > 0x0f)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid byte: %c", p);
            duk_throw(ctx);
        }
        q = src[j];
        b = reverseHexTable[q];
        if (b > 0x0f)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid byte: %c", q);
            duk_throw(ctx);
        }
        dst[i] = (a << 4) | b;
        i++;
    }
}
duk_ret_t native_iotjs_encoding_hex__decodeString(duk_context *ctx)
{
    duk_size_t sz_src;
    duk_uint8_t *src = (duk_uint8_t *)duk_require_lstring(ctx, 0, &sz_src);
    if (sz_src % 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
        duk_throw(ctx);
    }
    else if (sz_src > IOTJS_MAX_SAFE_INTEGER)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
        duk_throw(ctx);
    }
    duk_size_t sz_dst = sz_src / 2;
    duk_uint8_t *dst = duk_push_buffer(ctx, sz_dst, 0);
    if (sz_src)
    {
        native_iotjs_encoding_hex__decode_buffer(ctx, dst, src, sz_src);
    }
    return 1;
}
duk_ret_t native_iotjs_encoding_hex__decode(duk_context *ctx)
{
    duk_size_t sz_dst, sz_src;
    duk_uint8_t *dst = duk_require_buffer_data(ctx, 0, &sz_dst);
    duk_uint8_t *src = duk_require_buffer_data(ctx, 1, &sz_src);
    if (sz_src % 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
        duk_throw(ctx);
    }
    else if (sz_src > IOTJS_MAX_SAFE_INTEGER)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid src.length");
        duk_throw(ctx);
    }
    else if (sz_dst < sz_src / 2)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "encoding/hex: invalid dst.length");
        duk_throw(ctx);
    }
    if (sz_src)
    {
        native_iotjs_encoding_hex__decode_buffer(ctx, dst, src, sz_src);
        duk_push_uint(ctx, sz_src / 2);
    }
    else
    {
        duk_push_uint(ctx, 0);
    }
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