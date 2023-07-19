#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <iotjs/core/binary.h>
#include <iotjs/modules/js/base64.h>
#define IOTJSE_BASE64_DEFINE(name, key, n)                       \
    duk_push_object(ctx);                                        \
    duk_push_c_lightfunc(ctx, name##_encoded_len, 1, 1, 0);      \
    duk_put_prop_lstring(ctx, -2, "encodedLen", 10);             \
    duk_push_c_lightfunc(ctx, name##_encode, 1, 1, 0);           \
    duk_put_prop_lstring(ctx, -2, "encode", 6);                  \
    duk_push_c_lightfunc(ctx, name##_encode_to_string, 1, 1, 0); \
    duk_put_prop_lstring(ctx, -2, "encodeToString", 14);         \
    duk_push_c_lightfunc(ctx, name##_decoded_len, 1, 1, 0);      \
    duk_put_prop_lstring(ctx, -2, "decodedLen", 10);             \
    duk_push_c_lightfunc(ctx, name##_decode, 1, 1, 0);           \
    duk_put_prop_lstring(ctx, -2, "decode", 6);                  \
    duk_push_c_lightfunc(ctx, name##_decode_to_string, 1, 1, 0); \
    duk_put_prop_lstring(ctx, -2, "decodeToString", 14);         \
    duk_put_prop_lstring(ctx, -2, key, n);

#define BASE64_DEFINE(name, EncodedLen, Encode, DecodedLen, Decode)            \
    static duk_ret_t name##_encoded_len(duk_context *ctx)                      \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else if (duk_is_buffer_data(ctx, 0))                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);         \
        }                                                                      \
        unsigned int len = EncodedLen(src_len);                                \
        duk_push_number(ctx, len);                                             \
        return 1;                                                              \
    }                                                                          \
    static duk_ret_t name##_encode(duk_context *ctx)                           \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else if (duk_is_buffer_data(ctx, 0))                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);         \
        }                                                                      \
        unsigned int len = EncodedLen(src_len);                                \
        void *dst = duk_push_buffer(ctx, len, 0);                              \
        Encode(dst, src, src_len);                                             \
        return 1;                                                              \
    }                                                                          \
    static duk_ret_t name##_encode_to_string(duk_context *ctx)                 \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else if (duk_is_buffer_data(ctx, 0))                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);         \
        }                                                                      \
        unsigned int len = EncodedLen(src_len);                                \
        void *dst = duk_push_buffer(ctx, len, 0);                              \
        Encode(dst, src, src_len);                                             \
        duk_buffer_to_string(ctx, -1);                                         \
        return 1;                                                              \
    }                                                                          \
    static duk_ret_t name##_decoded_len(duk_context *ctx)                      \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else if (duk_is_buffer_data(ctx, 0))                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);         \
        }                                                                      \
        unsigned int len = DecodedLen(src_len);                                \
        duk_push_number(ctx, len);                                             \
        return 1;                                                              \
    }                                                                          \
    static duk_ret_t name##_decode(duk_context *ctx)                           \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        if (src_len)                                                           \
        {                                                                      \
            unsigned int len = DecodedLen(src_len);                            \
            void *dst = duk_push_buffer(ctx, len, 1);                          \
            unsigned int sz = Decode(dst, src, src_len);                       \
            if (!sz)                                                           \
            {                                                                  \
                duk_error(ctx, DUK_ERR_ERROR, "invalid base64 encoded value"); \
            }                                                                  \
            if (sz != len)                                                     \
            {                                                                  \
                duk_resize_buffer(ctx, -1, sz);                                \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            duk_push_buffer(ctx, 0, 0);                                        \
        }                                                                      \
        return 1;                                                              \
    }                                                                          \
    static duk_ret_t name##_decode_to_string(duk_context *ctx)                 \
    {                                                                          \
        duk_size_t src_len;                                                    \
        const void *src;                                                       \
        if (duk_is_string(ctx, 0))                                             \
        {                                                                      \
            src = duk_require_lstring(ctx, 0, &src_len);                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            src = duk_require_buffer_data(ctx, 0, &src_len);                   \
        }                                                                      \
        if (src_len)                                                           \
        {                                                                      \
            unsigned int len = DecodedLen(src_len);                            \
            void *dst = duk_push_buffer(ctx, len, 0);                          \
            unsigned int sz = Decode(dst, src, src_len);                       \
            if (!sz)                                                           \
            {                                                                  \
                duk_error(ctx, DUK_ERR_ERROR, "invalid base64 encoded value"); \
            }                                                                  \
            duk_buffer_to_string(ctx, -1);                                     \
            if (sz != len)                                                     \
            {                                                                  \
                duk_substring(ctx, -1, 0, sz);                                 \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            duk_push_lstring(ctx, "", 0);                                      \
        }                                                                      \
        return 1;                                                              \
    }

BASE64_DEFINE(std,
              iotjs_base64.std.encoded_len,
              iotjs_base64.std.encode,
              iotjs_base64.std.decoded_len,
              iotjs_base64.std.decode)
BASE64_DEFINE(raw_std,
              iotjs_base64.raw_std.encoded_len,
              iotjs_base64.raw_std.encode,
              iotjs_base64.raw_std.decoded_len,
              iotjs_base64.raw_std.decode)
BASE64_DEFINE(url,
              iotjs_base64.url.encoded_len,
              iotjs_base64.url.encode,
              iotjs_base64.url.decoded_len,
              iotjs_base64.url.decode)
BASE64_DEFINE(raw_url,
              iotjs_base64.raw_url.encoded_len,
              iotjs_base64.raw_url.encode,
              iotjs_base64.raw_url.decoded_len,
              iotjs_base64.raw_url.decode)

static duk_ret_t native_encodedLen(duk_context *ctx)
{
    unsigned int sz = duk_require_number(ctx, 0);
    duk_bool_t padding = duk_require_boolean(ctx, 1);
    duk_pop_2(ctx);
    duk_push_number(ctx, padding ? base64_encoded_len(sz) : base64_encoded_len_no_padding(sz));
    return 1;
}
static duk_ret_t native_decodedLen(duk_context *ctx)
{
    unsigned int sz = duk_require_number(ctx, 0);
    duk_bool_t padding = duk_require_boolean(ctx, 1);
    duk_pop_2(ctx);
    duk_push_number(ctx, padding ? base64_decoded_len(sz) : base64_decoded_len_no_padding(sz));
    return 1;
}
static duk_ret_t native_encode(duk_context *ctx)
{
    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 0))
    {
        src = duk_require_lstring(ctx, 0, &src_len);
    }
    else if (duk_is_buffer_data(ctx, 0))
    {
        src = duk_require_buffer_data(ctx, 0, &src_len);
    }
    else
    {
        src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);
    }
    uint8_t padding = 0;
    if (duk_is_string(ctx, 1))
    {
        padding = duk_require_lstring(ctx, 1, 0)[0];
    }
    const uint8_t *encode = duk_require_lstring(ctx, 2, 0);
    duk_pop_3(ctx);
    if (src_len)
    {
        duk_size_t dst_len = padding ? base64_encoded_len(src_len) : base64_encoded_len_no_padding(src_len);
        uint8_t *dst = duk_push_buffer(ctx, dst_len, 0);
        iotjs_base64_encode(dst, src, src_len, padding, encode);
    }
    else
    {
        duk_push_buffer(ctx, 0, 0);
    }
    return 1;
}
static duk_ret_t native_encodeToString(duk_context *ctx)
{
    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 0))
    {
        src = duk_require_lstring(ctx, 0, &src_len);
    }
    else if (duk_is_buffer_data(ctx, 0))
    {
        src = duk_require_buffer_data(ctx, 0, &src_len);
    }
    else
    {
        src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);
    }
    uint8_t padding = 0;
    if (duk_is_string(ctx, 1))
    {
        padding = duk_require_lstring(ctx, 1, 0)[0];
    }
    const uint8_t *encode = duk_require_lstring(ctx, 2, 0);
    duk_pop_3(ctx);
    if (src_len)
    {
        duk_size_t dst_len = padding ? base64_encoded_len(src_len) : base64_encoded_len_no_padding(src_len);
        uint8_t *dst = duk_push_buffer(ctx, dst_len, 0);
        iotjs_base64_encode(dst, src, src_len, padding, encode);
        duk_buffer_to_string(ctx, -1);
    }
    else
    {
        duk_push_lstring(ctx, "", 0);
    }
    return 1;
}
static duk_ret_t native_decode(duk_context *ctx)
{
    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 0))
    {
        src = duk_require_lstring(ctx, 0, &src_len);
    }
    else if (duk_is_buffer_data(ctx, 0))
    {
        src = duk_require_buffer_data(ctx, 0, &src_len);
    }
    else
    {
        src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);
    }
    uint8_t padding = 0;
    if (duk_is_string(ctx, 1))
    {
        padding = duk_require_lstring(ctx, 1, 0)[0];
    }
    const uint8_t *decode = duk_require_buffer_data(ctx, 2, 0);
    duk_require_callable(ctx, 3);
    duk_swap_top(ctx, 0);
    duk_pop_3(ctx);
    if (src_len)
    {
        duk_size_t dst_len = padding ? base64_decoded_len(src_len) : base64_decoded_len_no_padding(src_len);
        uint8_t *dst = duk_push_buffer(ctx, dst_len, 0);
        duk_size_t n = iotjs_base64_decode(dst, src, src_len, padding, decode);
        if (!n)
        {
            duk_error(ctx, DUK_ERR_ERROR, "invalid base64 encoded value");
        }
        else if (n != dst_len)
        {
            duk_push_number(ctx, n);
            duk_call(ctx, 2);
        }
    }
    else
    {
        duk_push_buffer(ctx, 0, 0);
    }
    return 1;
}
static duk_ret_t native_decodeToString(duk_context *ctx)
{
    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 0))
    {
        src = duk_require_lstring(ctx, 0, &src_len);
    }
    else if (duk_is_buffer_data(ctx, 0))
    {
        src = duk_require_buffer_data(ctx, 0, &src_len);
    }
    else
    {
        src = (const void *)duk_safe_to_lstring(ctx, 0, &src_len);
    }
    uint8_t padding = 0;
    if (duk_is_string(ctx, 1))
    {
        padding = duk_require_lstring(ctx, 1, 0)[0];
    }
    const uint8_t *decode = duk_require_buffer_data(ctx, 2, 0);
    duk_require_callable(ctx, 3);
    duk_swap_top(ctx, 0);
    duk_pop_3(ctx);
    if (src_len)
    {
        duk_size_t dst_len = padding ? base64_decoded_len(src_len) : base64_decoded_len_no_padding(src_len);
        uint8_t *dst = duk_push_buffer(ctx, dst_len, 0);
        duk_size_t n = iotjs_base64_decode(dst, src, src_len, padding, decode);
        if (!n)
        {
            duk_error(ctx, DUK_ERR_ERROR, "invalid base64 encoded value");
        }
        else if (n != dst_len)
        {
            duk_push_number(ctx, n);
            duk_call(ctx, 2);
        }
        duk_buffer_to_string(ctx, -1);
    }
    else
    {
        duk_push_lstring(ctx, "", 0);
    }
    return 1;
}

duk_ret_t native_iotjs_encoding_base64_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_base64_min_js, js_iotjs_modules_js_base64_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_undefined(ctx);
    duk_push_object(ctx);
    {
        duk_push_lstring(ctx, iotjs_base64_encodeStd, 64);
        duk_put_prop_lstring(ctx, -2, "encoder_std", 11);
        duk_push_lstring(ctx, iotjs_base64_encodeURL, 64);
        duk_put_prop_lstring(ctx, -2, "encoder_url", 11);

        void *dst = duk_push_buffer(ctx, 256, 0);
        memcpy(dst, iotjs_base64_decode_std, 256);
        duk_put_prop_lstring(ctx, -2, "decoder_std", 11);
        dst = duk_push_buffer(ctx, 256, 0);
        memcpy(dst, iotjs_base64_decode_url, 256);
        duk_put_prop_lstring(ctx, -2, "decoder_url", 11);

        duk_push_c_lightfunc(ctx, native_encodedLen, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "encodedLen", 10);
        duk_push_c_lightfunc(ctx, native_decodedLen, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "decodedLen", 10);

        duk_push_c_lightfunc(ctx, native_encode, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "encode", 6);
        duk_push_c_lightfunc(ctx, native_encodeToString, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "encodeToString", 14);

        duk_push_c_lightfunc(ctx, native_decode, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "decode", 6);
        duk_push_c_lightfunc(ctx, native_decodeToString, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "decodeToString", 14);
    }
    duk_call(ctx, 3);
    // IOTJSE_BASE64_DEFINE(std, "std", 3)
    // IOTJSE_BASE64_DEFINE(raw_std, "rawSTD", 6)
    // IOTJSE_BASE64_DEFINE(url, "url", 3)
    // IOTJSE_BASE64_DEFINE(raw_url, "rawURL", 6)
    return 0;
}