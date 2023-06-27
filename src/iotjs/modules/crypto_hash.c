#include <iotjs/core/finalizer.h>
#include <iotjs/core/memory.h>
#include <tomcrypt.h>
#include <iotjs/modules/js/hash.h>

#define VM_MODULE_HASH_RAW(HASH, SIZE, BLOCK)                                                                      \
    static void native_iotjs_crypto_##HASH##_free(void *p) { vm_free(p); }                                         \
    duk_ret_t native_iotjs_crypto_##HASH##_sum(duk_context *ctx)                                                   \
    {                                                                                                              \
        const char *in;                                                                                            \
        duk_size_t sz_in;                                                                                          \
        if (duk_is_string(ctx, 0))                                                                                 \
        {                                                                                                          \
            in = duk_require_lstring(ctx, 0, &sz_in);                                                              \
        }                                                                                                          \
        else                                                                                                       \
        {                                                                                                          \
            in = duk_require_buffer_data(ctx, 0, &sz_in);                                                          \
        }                                                                                                          \
        void *out = duk_push_buffer(ctx, SIZE, 0);                                                                 \
                                                                                                                   \
        hash_state md;                                                                                             \
        if (HASH##_init(&md))                                                                                      \
        {                                                                                                          \
            duk_pop_2(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_init error", #HASH);                                                 \
        }                                                                                                          \
        if (HASH##_process(&md, in, sz_in))                                                                        \
        {                                                                                                          \
            duk_pop_2(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_process error", #HASH);                                              \
        }                                                                                                          \
        if (HASH##_done(&md, out))                                                                                 \
        {                                                                                                          \
            duk_pop_2(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_done error", #HASH);                                                 \
        }                                                                                                          \
        return 1;                                                                                                  \
    }                                                                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_clone(duk_context *ctx)                                                 \
    {                                                                                                              \
        finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(hash_state));                                   \
        finalizer->free = vm_free;                                                                                 \
        hash_state *md = finalizer->p;                                                                             \
        if (duk_is_null_or_undefined(ctx, 0))                                                                      \
        {                                                                                                          \
            if (HASH##_init(md))                                                                                   \
            {                                                                                                      \
                duk_pop_2(ctx);                                                                                    \
                duk_error(ctx, DUK_ERR_ERROR, "%s_init error", #HASH);                                             \
            }                                                                                                      \
        }                                                                                                          \
        else                                                                                                       \
        {                                                                                                          \
            finalizer = vm_require_finalizer(ctx, 0, native_iotjs_crypto_##HASH##_free);                           \
            hash_state *src = finalizer->p;                                                                        \
            memcpy(md, src, sizeof(hash_state));                                                                   \
        }                                                                                                          \
        return 1;                                                                                                  \
    }                                                                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_reset(duk_context *ctx)                                                 \
    {                                                                                                              \
        finalizer_t *finalizer = vm_require_finalizer(ctx, 0, native_iotjs_crypto_##HASH##_free);                  \
        hash_state *md = finalizer->p;                                                                             \
        if (HASH##_init(md))                                                                                       \
        {                                                                                                          \
            duk_pop_2(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_reset error", #HASH);                                                \
        }                                                                                                          \
        return 0;                                                                                                  \
    }                                                                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_write(duk_context *ctx)                                                 \
    {                                                                                                              \
        const char *in;                                                                                            \
        duk_size_t sz_in;                                                                                          \
        if (duk_is_string(ctx, -1))                                                                                \
        {                                                                                                          \
            in = duk_require_lstring(ctx, -1, &sz_in);                                                             \
        }                                                                                                          \
        else                                                                                                       \
        {                                                                                                          \
            in = duk_require_buffer_data(ctx, -1, &sz_in);                                                         \
        }                                                                                                          \
                                                                                                                   \
        finalizer_t *finalizer = vm_require_finalizer(ctx, 0, native_iotjs_crypto_##HASH##_free);                  \
        hash_state *md = finalizer->p;                                                                             \
        if (HASH##_process(md, in, sz_in))                                                                         \
        {                                                                                                          \
            duk_pop_3(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_process error", #HASH);                                              \
        }                                                                                                          \
        duk_pop_3(ctx);                                                                                            \
        duk_push_number(ctx, sz_in);                                                                               \
        return 1;                                                                                                  \
    }                                                                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_done(duk_context *ctx)                                                  \
    {                                                                                                              \
        finalizer_t *finalizer = vm_require_finalizer(ctx, 0, native_iotjs_crypto_##HASH##_free);                  \
        hash_state *md = finalizer->p;                                                                             \
        void *out = duk_push_buffer(ctx, SIZE, 0);                                                                 \
        if (HASH##_done(md, out))                                                                                  \
        {                                                                                                          \
            duk_pop_2(ctx);                                                                                        \
            duk_error(ctx, DUK_ERR_ERROR, "%s_done error", #HASH);                                                 \
        }                                                                                                          \
        return 1;                                                                                                  \
    }                                                                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_init(duk_context *ctx)                                                  \
    {                                                                                                              \
        duk_swap(ctx, 0, 1);                                                                                       \
        duk_pop_2(ctx);                                                                                            \
        duk_push_c_lightfunc(ctx, native_iotjs_crypto_##HASH##_sum, 1, 1, 0);                                      \
        duk_put_prop_lstring(ctx, -2, "sum", 3);                                                                   \
                                                                                                                   \
        duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_hash_min_js, js_iotjs_modules_js_hash_min_js_len); \
        duk_swap_top(ctx, -2);                                                                                     \
                                                                                                                   \
        duk_push_object(ctx);                                                                                      \
        duk_push_number(ctx, SIZE);                                                                                \
        duk_put_prop_lstring(ctx, -2, "size", 4);                                                                  \
        duk_push_number(ctx, BLOCK);                                                                               \
        duk_put_prop_lstring(ctx, -2, "block", 5);                                                                 \
        duk_push_c_lightfunc(ctx, native_iotjs_crypto_##HASH##_clone, 1, 1, 0);                                    \
        duk_put_prop_lstring(ctx, -2, "clone", 5);                                                                 \
        duk_push_c_lightfunc(ctx, native_iotjs_crypto_##HASH##_reset, 1, 1, 0);                                    \
        duk_put_prop_lstring(ctx, -2, "reset", 5);                                                                 \
        duk_push_c_lightfunc(ctx, native_iotjs_crypto_##HASH##_write, 2, 2, 0);                                    \
        duk_put_prop_lstring(ctx, -2, "write", 5);                                                                 \
        duk_push_c_lightfunc(ctx, native_iotjs_crypto_##HASH##_done, 1, 1, 0);                                     \
        duk_put_prop_lstring(ctx, -2, "done", 4);                                                                  \
                                                                                                                   \
        duk_push_undefined(ctx);                                                                                   \
        duk_swap_top(ctx, -2);                                                                                     \
                                                                                                                   \
        duk_call(ctx, 3);                                                                                          \
        return 0;                                                                                                  \
    }
#define VM_MODULE_HASH(HASH) VM_MODULE_HASH_RAW(HASH, HASH##_desc.hashsize, HASH##_desc.blocksize)

VM_MODULE_HASH(md4)
VM_MODULE_HASH(md5)
VM_MODULE_HASH(sha1)
VM_MODULE_HASH(sha224)
VM_MODULE_HASH(sha256)
VM_MODULE_HASH(sha384)
VM_MODULE_HASH(sha512)
VM_MODULE_HASH(sha512_224)
VM_MODULE_HASH(sha512_256)

int iotjs_module_crypto_sha1(const unsigned char *in, unsigned long inlen, unsigned char *hash)
{
    int err;
    hash_state md;
    err = sha1_init(&md);
    if (err)
    {
        return err;
    }
    err = sha1_process(&md, in, inlen);
    if (err)
    {
        return err;
    }
    err = sha1_done(&md, hash);
    if (err)
    {
        return err;
    }
    return 0;
}
