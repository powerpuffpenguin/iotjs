#include <iotjs/core/defines.h>
#include <iotjs/modules/js/crypto_cipher.h>
#include <iotjs/core/module.h>
#include <iotjs/core/memory.h>
#include <tomcrypt.h>

#ifndef CRYPT_INVALID_IVSIZE
#define CRYPT_INVALID_IVSIZE 100
#endif

#define __IOTJS_CRYPTO_GET_ECH__(ctx)                         \
    duk_get_prop_lstring(ctx, 0, "cipher", 6);                \
    int cipher = find_cipher_id(duk_require_number(ctx, -1)); \
    if (cipher < 0)                                           \
    {                                                         \
        duk_pop_2(ctx);                                       \
        duk_push_array(ctx);                                  \
        duk_push_int(ctx, CRYPT_INVALID_CIPHER);              \
        duk_put_prop_index(ctx, -2, 1);                       \
        return 1;                                             \
    }                                                         \
    duk_pop(ctx);                                             \
    const void *key;                                          \
    duk_size_t key_len;                                       \
    duk_get_prop_lstring(ctx, 0, "key", 3);                   \
    if (duk_is_string(ctx, -1))                               \
    {                                                         \
        key = duk_require_lstring(ctx, -1, &key_len);         \
    }                                                         \
    else                                                      \
    {                                                         \
        key = duk_require_buffer_data(ctx, -1, &key_len);     \
    }                                                         \
    duk_pop(ctx);                                             \
    duk_get_prop_lstring(ctx, 0, "rounds", 6);                \
    int rounds = duk_require_number(ctx, -1);                 \
    duk_pop(ctx)

#define __IOTJS_CRYPTO_GET_IV__(ctx)                      \
    const void *iv;                                       \
    duk_size_t iv_len;                                    \
    duk_get_prop_lstring(ctx, 0, "iv", 2);                \
    if (duk_is_string(ctx, -1))                           \
    {                                                     \
        iv = duk_require_lstring(ctx, -1, &iv_len);       \
    }                                                     \
    else                                                  \
    {                                                     \
        iv = duk_require_buffer_data(ctx, -1, &iv_len);   \
    }                                                     \
    duk_pop(ctx);                                         \
    if (iv_len != cipher_descriptor[cipher].block_length) \
    {                                                     \
        duk_push_array(ctx);                              \
        duk_push_int(ctx, CRYPT_INVALID_IVSIZE);          \
        duk_put_prop_index(ctx, -2, 1);                   \
        return 1;                                         \
    }

static duk_ret_t native_ecb(duk_context *ctx)
{
    __IOTJS_CRYPTO_GET_ECH__(ctx);

    duk_push_array(ctx);
    symmetric_ECB *ecb = duk_push_fixed_buffer(ctx, sizeof(symmetric_ECB));
    duk_put_prop_index(ctx, -2, 0);
    duk_push_int(ctx, ecb_start(cipher, key, key_len, rounds, ecb));
    duk_put_prop_index(ctx, -2, 1);
    return 1;
}
static duk_ret_t native_ecb_memory(duk_context *ctx)
{
    symmetric_ECB *ecb = duk_require_buffer_data(ctx, 0, 0);

    duk_size_t dst_len;
    void *dst = duk_require_buffer_data(ctx, 1, &dst_len);

    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 2))
    {
        src = duk_require_lstring(ctx, 2, &src_len);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 2, &src_len);
    }
    if (dst_len < src_len)
    {
        duk_pop_3(ctx);
        duk_push_number(ctx, CRYPT_BUFFER_OVERFLOW);
        return 1;
    }

    int ret = duk_require_boolean(ctx, 3) ? ecb_encrypt(src, dst, src_len, ecb) : ecb_decrypt(src, dst, src_len, ecb);
    duk_pop_n(ctx, 4);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t native_cbc(duk_context *ctx)
{
    __IOTJS_CRYPTO_GET_ECH__(ctx);
    __IOTJS_CRYPTO_GET_IV__(ctx);

    duk_push_array(ctx);
    symmetric_CBC *cbc = duk_push_fixed_buffer(ctx, sizeof(symmetric_CBC));
    duk_put_prop_index(ctx, -2, 0);
    duk_push_int(ctx, cbc_start(cipher,
                                iv,
                                key, key_len, rounds, cbc));
    duk_put_prop_index(ctx, -2, 1);
    return 1;
}
static duk_ret_t native_cbc_memory(duk_context *ctx)
{
    symmetric_CBC *cbc = duk_require_buffer_data(ctx, 0, 0);

    duk_size_t dst_len;
    void *dst = duk_require_buffer_data(ctx, 1, &dst_len);

    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 2))
    {
        src = duk_require_lstring(ctx, 2, &src_len);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 2, &src_len);
    }
    if (dst_len < src_len)
    {
        duk_pop_3(ctx);
        duk_push_number(ctx, CRYPT_BUFFER_OVERFLOW);
        return 1;
    }

    int ret = duk_require_boolean(ctx, 3) ? cbc_encrypt(src, dst, src_len, cbc) : cbc_decrypt(src, dst, src_len, cbc);
    duk_pop_n(ctx, 4);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t native_cfb(duk_context *ctx)
{
    __IOTJS_CRYPTO_GET_ECH__(ctx);
    __IOTJS_CRYPTO_GET_IV__(ctx);

    duk_push_array(ctx);
    symmetric_CFB *cfb = duk_push_fixed_buffer(ctx, sizeof(symmetric_CFB));
    duk_put_prop_index(ctx, -2, 0);
    duk_push_int(ctx, cfb_start(cipher,
                                iv,
                                key, key_len, rounds, cfb));
    duk_put_prop_index(ctx, -2, 1);
    return 1;
}
static duk_ret_t native_cfb_memory(duk_context *ctx)
{
    symmetric_CFB *cfb = duk_require_buffer_data(ctx, 0, 0);

    duk_size_t dst_len;
    void *dst = duk_require_buffer_data(ctx, 1, &dst_len);

    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 2))
    {
        src = duk_require_lstring(ctx, 2, &src_len);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 2, &src_len);
    }
    if (dst_len < src_len)
    {
        duk_pop_3(ctx);
        duk_push_number(ctx, CRYPT_BUFFER_OVERFLOW);
        return 1;
    }

    int ret = duk_require_boolean(ctx, 3) ? cfb_encrypt(src, dst, src_len, cfb) : cfb_decrypt(src, dst, src_len, cfb);
    duk_pop_n(ctx, 4);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t native_ofb(duk_context *ctx)
{
    __IOTJS_CRYPTO_GET_ECH__(ctx);
    __IOTJS_CRYPTO_GET_IV__(ctx);

    duk_push_array(ctx);
    symmetric_OFB *ofb = duk_push_fixed_buffer(ctx, sizeof(symmetric_OFB));
    duk_put_prop_index(ctx, -2, 0);
    duk_push_int(ctx, ofb_start(cipher,
                                iv,
                                key, key_len, rounds, ofb));
    duk_put_prop_index(ctx, -2, 1);
    return 1;
}
static duk_ret_t native_ofb_memory(duk_context *ctx)
{
    symmetric_OFB *ofb = duk_require_buffer_data(ctx, 0, 0);

    duk_size_t dst_len;
    void *dst = duk_require_buffer_data(ctx, 1, &dst_len);

    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 2))
    {
        src = duk_require_lstring(ctx, 2, &src_len);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 2, &src_len);
    }
    if (dst_len < src_len)
    {
        duk_pop_3(ctx);
        duk_push_number(ctx, CRYPT_BUFFER_OVERFLOW);
        return 1;
    }

    int ret = duk_require_boolean(ctx, 3) ? ofb_encrypt(src, dst, src_len, ofb) : ofb_decrypt(src, dst, src_len, ofb);
    duk_pop_n(ctx, 4);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t native_ctr(duk_context *ctx)
{
    __IOTJS_CRYPTO_GET_ECH__(ctx);
    __IOTJS_CRYPTO_GET_IV__(ctx);

    duk_get_prop_lstring(ctx, 0, "mode", 4);
    int mode = duk_require_int(ctx, -1);
    duk_pop(ctx);

    duk_push_array(ctx);
    symmetric_CTR *ctr = duk_push_fixed_buffer(ctx, sizeof(symmetric_CTR));
    duk_put_prop_index(ctx, -2, 0);
    duk_push_int(ctx, ctr_start(cipher,
                                iv,
                                key, key_len, rounds, mode, ctr));
    duk_put_prop_index(ctx, -2, 1);
    return 1;
}
static duk_ret_t native_ctr_memory(duk_context *ctx)
{
    symmetric_CTR *ctr = duk_require_buffer_data(ctx, 0, 0);

    duk_size_t dst_len;
    void *dst = duk_require_buffer_data(ctx, 1, &dst_len);

    duk_size_t src_len;
    const void *src;
    if (duk_is_string(ctx, 2))
    {
        src = duk_require_lstring(ctx, 2, &src_len);
    }
    else
    {
        src = duk_require_buffer_data(ctx, 2, &src_len);
    }
    if (dst_len < src_len)
    {
        duk_pop_3(ctx);
        duk_push_number(ctx, CRYPT_BUFFER_OVERFLOW);
        return 1;
    }

    int ret = duk_require_boolean(ctx, 3) ? ctr_encrypt(src, dst, src_len, ctr) : ctr_decrypt(src, dst, src_len, ctr);
    duk_pop_n(ctx, 4);
    duk_push_number(ctx, ret);
    return 1;
}

duk_ret_t native_iotjs_crypto_cipher_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_crypto_cipher_min_js, js_iotjs_modules_js_crypto_cipher_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_object(ctx);
    {
        duk_push_number(ctx, aes_desc.ID);
        duk_put_prop_lstring(ctx, -2, "AES", 3);

        duk_push_int(ctx, CRYPT_OK);
        duk_put_prop_string(ctx, -2, "CRYPT_OK");
        duk_push_int(ctx, CRYPT_ERROR);
        duk_put_prop_string(ctx, -2, "CRYPT_ERROR");
        duk_push_int(ctx, CRYPT_NOP);
        duk_put_prop_string(ctx, -2, "CRYPT_NOP");
        duk_push_int(ctx, CRYPT_INVALID_KEYSIZE);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_KEYSIZE");
        duk_push_int(ctx, CRYPT_INVALID_ROUNDS);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_ROUNDS");
        duk_push_int(ctx, CRYPT_FAIL_TESTVECTOR);
        duk_put_prop_string(ctx, -2, "CRYPT_FAIL_TESTVECTOR");
        duk_push_int(ctx, CRYPT_BUFFER_OVERFLOW);
        duk_put_prop_string(ctx, -2, "CRYPT_BUFFER_OVERFLOW");
        duk_push_int(ctx, CRYPT_INVALID_PACKET);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_PACKET");
        duk_push_int(ctx, CRYPT_INVALID_PRNGSIZE);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_PRNGSIZE");
        duk_push_int(ctx, CRYPT_ERROR_READPRNG);
        duk_put_prop_string(ctx, -2, "CRYPT_ERROR_READPRNG");
        duk_push_int(ctx, CRYPT_INVALID_CIPHER);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_CIPHER");
        duk_push_int(ctx, CRYPT_INVALID_HASH);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_HASH");
        duk_push_int(ctx, CRYPT_INVALID_PRNG);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_PRNG");
        duk_push_int(ctx, CRYPT_MEM);
        duk_put_prop_string(ctx, -2, "CRYPT_MEM");
        duk_push_int(ctx, CRYPT_PK_TYPE_MISMATCH);
        duk_put_prop_string(ctx, -2, "CRYPT_PK_TYPE_MISMATCH");
        duk_push_int(ctx, CRYPT_PK_NOT_PRIVATE);
        duk_put_prop_string(ctx, -2, "CRYPT_PK_NOT_PRIVATE");
        duk_push_int(ctx, CRYPT_INVALID_ARG);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_ARG");
        duk_push_int(ctx, CRYPT_FILE_NOTFOUND);
        duk_put_prop_string(ctx, -2, "CRYPT_FILE_NOTFOUND");
        duk_push_int(ctx, CRYPT_PK_INVALID_TYPE);
        duk_put_prop_string(ctx, -2, "CRYPT_PK_INVALID_TYPE");
        duk_push_int(ctx, CRYPT_OVERFLOW);
        duk_put_prop_string(ctx, -2, "CRYPT_OVERFLOW");
        duk_push_int(ctx, CRYPT_UNUSED1);
        duk_put_prop_string(ctx, -2, "CRYPT_UNUSED1");
        duk_push_int(ctx, CRYPT_INPUT_TOO_LONG);
        duk_put_prop_string(ctx, -2, "CRYPT_INPUT_TOO_LONG");
        duk_push_int(ctx, CRYPT_PK_INVALID_SIZE);
        duk_put_prop_string(ctx, -2, "CRYPT_PK_INVALID_SIZE");
        duk_push_int(ctx, CRYPT_INVALID_PRIME_SIZE);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_PRIME_SIZE");
        duk_push_int(ctx, CRYPT_PK_INVALID_PADDING);
        duk_put_prop_string(ctx, -2, "CRYPT_PK_INVALID_PADDING");
        duk_push_int(ctx, CRYPT_HASH_OVERFLOW);
        duk_put_prop_string(ctx, -2, "CRYPT_HASH_OVERFLOW");

        duk_push_int(ctx, CRYPT_INVALID_IVSIZE);
        duk_put_prop_string(ctx, -2, "CRYPT_INVALID_IVSIZE");

        duk_push_int(ctx, CTR_COUNTER_BIG_ENDIAN);
        duk_put_prop_string(ctx, -2, "CTR_COUNTER_BIG_ENDIAN");
        duk_push_int(ctx, CTR_COUNTER_LITTLE_ENDIAN);
        duk_put_prop_string(ctx, -2, "CTR_COUNTER_LITTLE_ENDIAN");
        duk_push_int(ctx, LTC_CTR_RFC3686);
        duk_put_prop_string(ctx, -2, "LTC_CTR_RFC3686");

        duk_push_c_lightfunc(ctx, native_ecb, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "ecb", 3);
        duk_push_c_lightfunc(ctx, native_ecb_memory, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "ecb_memory", 10);

        duk_push_c_lightfunc(ctx, native_cbc, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "cbc", 3);
        duk_push_c_lightfunc(ctx, native_cbc_memory, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "cbc_memory", 10);

        duk_push_c_lightfunc(ctx, native_cfb, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "cfb", 3);
        duk_push_c_lightfunc(ctx, native_cfb_memory, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "cfb_memory", 10);

        duk_push_c_lightfunc(ctx, native_ofb, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "ofb", 3);
        duk_push_c_lightfunc(ctx, native_ofb_memory, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "ofb_memory", 10);

        duk_push_c_lightfunc(ctx, native_ctr, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "ctr", 3);
        duk_push_c_lightfunc(ctx, native_ctr_memory, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "ctr_memory", 10);
    }

    duk_call(ctx, 3);
    return 0;
}