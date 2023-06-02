#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>

// 爲棧頂的 n 個元素創建一個快照，存儲到 heap_stash["snapshot"][bucket][key] 中
void vm_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n);
// 將存儲的快照恢復到棧頂
// del_snapshot 爲 true 則在恢復後刪除快照
void vm_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot);

// ... args, class Completer => ... promise
// 1. new Completer()..args=args
// 2. push completer to heap_stash
// 3. return completer.promise
void vm_async_completer_args(duk_context *ctx, void *key);

// ... value/error => ...
// 完成 completer 以 resolve/reject 回覆 promise
void vm_async_complete(duk_context *ctx, void *key, duk_bool_t ok, duk_bool_t get);
// ... value => ...
// 完成 completer 以 resolve 回覆 promise
#define vm_async_resolve(ctx, key) vm_async_complete(ctx, key, 1, 0);
// ... error => ...
// 完成 completer 以 reject 回覆 promise
#define vm_async_reject(ctx, key) vm_async_complete(ctx, key, 0, 0);
// ... value => ... completer
// 完成 completer 以 resolve 回覆 promise
#define vm_async_resolve_get(ctx, key) vm_async_complete(ctx, key, 1, 1);
// ... error => ... completer
// 完成 completer 以 reject 回覆 promise
#define vm_async_reject_get(ctx, key) vm_async_complete(ctx, key, 0, 1);

#endif