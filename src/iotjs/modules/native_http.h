#ifndef IOTJS_MODULES_NATIVE_HTTP_H
#define IOTJS_MODULES_NATIVE_HTTP_H
#include <duktape.h>
duk_ret_t native_http_parse_url(duk_context *ctx);
duk_ret_t native_ws_key(duk_context *ctx);
duk_ret_t native_http_expand_ws(duk_context *ctx);
#endif
