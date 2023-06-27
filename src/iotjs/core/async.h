#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>

// 爲棧頂的 n 個元素創建一個快照，存儲到 heap_stash["snapshot"][bucket][key] 中
duk_context *vm_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n);
duk_context *vm_snapshot_copy(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n);

// 如果快照存在，將存儲的快照恢復到棧頂
// del_snapshot 爲 true 則在恢復後刪除快照
// 恢復失敗說明內存不足應該結束程序，否則可能導致永久的內存泄漏，通常只應該在異步回調中用於恢復棧
duk_context *vm_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot);
// 在返回快照環境，不存在 duk_err
duk_context *vm_require_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key);

// 如果快照存在則刪除快照
void vm_remove_snapshot(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key);

// // 爲異步 api 創建 promise，並爲棧參數創建快照
// // ... v0, v1, ... vn => promise
// duk_context *vm_new_completer(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n);

// // ... completer, value => ... completer, return
// // 完成 completer 以 resolve 回覆 promise
// void vm_resolve(duk_context *ctx, duk_idx_t obj_idx);
// // ... completer, error => ... completer, return
// // 完成 completer 以 reject 回覆 promise
// void vm_reject(duk_context *ctx, duk_idx_t obj_idx);

// // ... => ... val
// // duk_call f(arg)
// void vm_complete_lightfunc(duk_context *ctx, duk_c_function f, void *arg);
// // ... => ...
// // duk_call f(arg)
// void vm_complete_lightfunc_noresult(duk_context *ctx, duk_c_function f, void *arg);

// // ... args, class Completer => ... promise
// // 1. new Completer()..args=args
// // 2. push completer to heap_stash
// // 3. return completer.promise
// void vm_async_completer_args(duk_context *ctx, void *key);

// // ... value/error => ...
// // 完成 completer 以 resolve/reject 回覆 promise
// void vm_async_complete(duk_context *ctx, void *key, duk_bool_t ok, duk_bool_t get);
// // ... value => ...
// // 完成 completer 以 resolve 回覆 promise
// #define vm_async_resolve(ctx, key) vm_async_complete(ctx, key, 1, 0);
// // ... error => ...
// // 完成 completer 以 reject 回覆 promise
// #define vm_async_reject(ctx, key) vm_async_complete(ctx, key, 0, 0);
// // ... value => ... completer
// // 完成 completer 以 resolve 回覆 promise
// #define vm_async_resolve_get(ctx, key) vm_async_complete(ctx, key, 1, 1);
// // ... error => ... completer
// // 完成 completer 以 reject 回覆 promise
// #define vm_async_reject_get(ctx, key) vm_async_complete(ctx, key, 0, 1);

#endif