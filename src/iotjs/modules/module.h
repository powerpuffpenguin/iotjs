#ifndef IOTJS_MODULES_H
#define IOTJS_MODULES_H
#include <duktape.h>
void __iotjs_modules_init();
duk_ret_t native_iotjs_init(duk_context *ctx);
duk_ret_t native_iotjs_coroutine_init(duk_context *ctx);
duk_ret_t native_iotjs_encoding_hex_init(duk_context *ctx);
duk_ret_t native_iotjs_encoding_base64_init(duk_context *ctx);

duk_ret_t native_iotjs_crypto_md4_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_md5_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha1_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha224_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha256_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha384_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha512_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha512_224_init(duk_context *ctx);
duk_ret_t native_iotjs_crypto_sha512_256_init(duk_context *ctx);

duk_ret_t native_iotjs_fs_init(duk_context *ctx);
duk_ret_t native_iotjs_net_init(duk_context *ctx);
duk_ret_t native_iotjs_net_http_init(duk_context *ctx);

duk_ret_t native_iotjs_bytes_init(duk_context *ctx);
duk_ret_t native_iotjs_mtd_init(duk_context *ctx);
#endif
