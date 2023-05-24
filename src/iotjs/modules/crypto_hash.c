#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <tomcrypt.h>
#include <iotjs/modules/js/tsc.hash.h>

#define VM_MODULE_HASH(HASH, SIZE, BLOCK)                                                              \
    duk_ret_t native_iotjs_crypto_##HASH##_sum(duk_context *ctx)                                       \
    {                                                                                                  \
        const char *in;                                                                                \
        duk_size_t sz_in;                                                                              \
        if (duk_is_string(ctx, 0))                                                                     \
        {                                                                                              \
            in = duk_require_lstring(ctx, 0, &sz_in);                                                  \
        }                                                                                              \
        else                                                                                           \
        {                                                                                              \
            in = duk_require_buffer_data(ctx, 0, &sz_in);                                              \
        }                                                                                              \
        void *out = duk_push_buffer(ctx, SIZE, 0);                                                     \
                                                                                                       \
        hash_state md;                                                                                 \
        if (HASH##_init(&md))                                                                          \
        {                                                                                              \
            duk_pop_2(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_init error", #HASH);                         \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        if (HASH##_process(&md, in, sz_in))                                                            \
        {                                                                                              \
            duk_pop_2(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_process error", #HASH);                      \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        if (HASH##_done(&md, out))                                                                     \
        {                                                                                              \
            duk_pop_2(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_done error", #HASH);                         \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        return 1;                                                                                      \
    }                                                                                                  \
    duk_ret_t native_iotjs_crypto_##HASH##_clone(duk_context *ctx)                                     \
    {                                                                                                  \
        hash_state *src = NULL;                                                                        \
        if (!duk_is_undefined(ctx, 0))                                                                 \
        {                                                                                              \
            duk_get_prop_lstring(ctx, -1, "ptr", 3);                                                   \
            src = duk_require_pointer(ctx, -1);                                                        \
            duk_pop(ctx);                                                                              \
        }                                                                                              \
        hash_state *md = vm_malloc_with_finalizer(ctx, sizeof(hash_state), NULL);                      \
        if (src)                                                                                       \
        {                                                                                              \
            memcpy(md, src, sizeof(hash_state));                                                       \
        }                                                                                              \
        else                                                                                           \
        {                                                                                              \
            if (HASH##_init(md))                                                                       \
            {                                                                                          \
                duk_pop_2(ctx);                                                                        \
                duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_init error", #HASH);                     \
                duk_throw(ctx);                                                                        \
            }                                                                                          \
        }                                                                                              \
        return 1;                                                                                      \
    }                                                                                                  \
    duk_ret_t native_iotjs_crypto_##HASH##_reset(duk_context *ctx)                                     \
    {                                                                                                  \
        duk_get_prop_lstring(ctx, -1, "ptr", 3);                                                       \
        hash_state *md = duk_require_pointer(ctx, -1);                                                 \
        if (HASH##_init(md))                                                                           \
        {                                                                                              \
            duk_pop_2(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_reset error", #HASH);                        \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        return 0;                                                                                      \
    }                                                                                                  \
    duk_ret_t native_iotjs_crypto_##HASH##_write(duk_context *ctx)                                     \
    {                                                                                                  \
        const char *in;                                                                                \
        duk_size_t sz_in;                                                                              \
        if (duk_is_string(ctx, -1))                                                                    \
        {                                                                                              \
            in = duk_require_lstring(ctx, -1, &sz_in);                                                 \
        }                                                                                              \
        else                                                                                           \
        {                                                                                              \
            in = duk_require_buffer_data(ctx, -1, &sz_in);                                             \
        }                                                                                              \
                                                                                                       \
        duk_get_prop_lstring(ctx, 0, "ptr", 3);                                                        \
        hash_state *md = duk_require_pointer(ctx, -1);                                                 \
        if (HASH##_process(md, in, sz_in))                                                             \
        {                                                                                              \
            duk_pop_3(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_process error", #HASH);                      \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        duk_pop_3(ctx);                                                                                \
        duk_push_number(ctx, sz_in);                                                                   \
        return 1;                                                                                      \
    }                                                                                                  \
    duk_ret_t native_iotjs_crypto_##HASH##_done(duk_context *ctx)                                      \
    {                                                                                                  \
        duk_get_prop_lstring(ctx, 0, "ptr", 3);                                                        \
        hash_state *md = duk_require_pointer(ctx, -1);                                                 \
        void *out = duk_push_buffer(ctx, SIZE, 0);                                                     \
        if (HASH##_done(md, out))                                                                      \
        {                                                                                              \
            duk_pop_2(ctx);                                                                            \
            duk_push_error_object(ctx, DUK_ERR_ERROR, "%s_done error", #HASH);                         \
            duk_throw(ctx);                                                                            \
        }                                                                                              \
        return 1;                                                                                      \
    }                                                                                                  \
    duk_ret_t native_iotjs_crypto_##HASH##_init(duk_context *ctx)                                      \
    {                                                                                                  \
        duk_swap(ctx, 0, 1);                                                                           \
        duk_pop_2(ctx);                                                                                \
        duk_push_c_function(ctx, native_iotjs_crypto_##HASH##_sum, 1);                                 \
        duk_put_prop_lstring(ctx, -2, "sum", 3);                                                       \
                                                                                                       \
        duk_eval_lstring(ctx, iotjs_modules_js_tsc_hash_min_js, iotjs_modules_js_tsc_hash_min_js_len); \
        duk_swap_top(ctx, -2);                                                                         \
                                                                                                       \
        duk_push_object(ctx);                                                                          \
        duk_push_number(ctx, SIZE);                                                                    \
        duk_put_prop_lstring(ctx, -2, "size", 4);                                                      \
        duk_push_number(ctx, BLOCK);                                                                   \
        duk_put_prop_lstring(ctx, -2, "block", 5);                                                     \
        duk_push_c_function(ctx, native_iotjs_crypto_##HASH##_clone, 1);                               \
        duk_put_prop_lstring(ctx, -2, "clone", 5);                                                     \
        duk_push_c_function(ctx, native_iotjs_crypto_##HASH##_reset, 1);                               \
        duk_put_prop_lstring(ctx, -2, "reset", 5);                                                     \
        duk_push_c_function(ctx, native_iotjs_crypto_##HASH##_write, 2);                               \
        duk_put_prop_lstring(ctx, -2, "write", 5);                                                     \
        duk_push_c_function(ctx, native_iotjs_crypto_##HASH##_done, 2);                                \
        duk_put_prop_lstring(ctx, -2, "done", 4);                                                      \
                                                                                                       \
        duk_push_undefined(ctx);                                                                       \
        duk_swap_top(ctx, -2);                                                                         \
                                                                                                       \
        duk_call(ctx, 3);                                                                              \
        return 0;                                                                                      \
    }

VM_MODULE_HASH(md4, 16, 64)
VM_MODULE_HASH(md5, 16, 64)
VM_MODULE_HASH(sha1, 20, 64)
VM_MODULE_HASH(sha224, 28, 64)
VM_MODULE_HASH(sha256, 32, 64)
VM_MODULE_HASH(sha384, 48, 64)
VM_MODULE_HASH(sha512, 64, 64)
VM_MODULE_HASH(sha512_224, 28, 64)
VM_MODULE_HASH(sha512_256, 32, 64)
