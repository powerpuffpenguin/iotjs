#ifndef IOTJS_MODULES_H
#define IOTJS_MODULES_H
#include <duktape.h>

duk_ret_t native_iotjs_init(duk_context *ctx);
duk_ret_t native_iotjs_fs_init(duk_context *ctx);

#endif
