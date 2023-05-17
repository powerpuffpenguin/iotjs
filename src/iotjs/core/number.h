#ifndef IOTJS_CORE_NUMBER_H
#define IOTJS_CORE_NUMBER_H
#include <duktape.h>
// push size_t，如果值太大就以字符串的形式存儲
// ... => ... number|string
void vm_push_size(duk_context *ctx, size_t val);
void vm_push_uint64(duk_context *ctx, uint64_t val);
void vm_push_int64(duk_context *ctx, int64_t val);
#endif