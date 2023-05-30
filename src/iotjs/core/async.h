#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>
// ... args, class completer => ... promise
// 1. new Completer()..args=args
// 2. push completer to heas
// 3. return completer.promise
void vm_async_completer_args(duk_context *ctx, void *key);

void vm_async_complete(duk_context *ctx, void *key, duk_bool_t ok);

// ... error => ...
// 完成 completer 以 reject 回覆 promise
#define vm_async_reject(ctx, key) vm_async_complete(ctx, key, 0);
// ... value => ...
// 完成 completer 以 resolve 回覆 promise
#define vm_async_resolve(ctx, key) vm_async_complete(ctx, key, 1);
#endif