#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>
#include <iotjs/core/js.h>

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

typedef struct
{
    /* data */
} vm_job_t;

typedef void (*vm_async_work)(vm_context_t *vm, void *args);
// 將 vm_async_work 放到工作線程中執行
// duk_throw ... => ...
void vm_async(duk_context *ctx, vm_async_work work, void *args);

#endif