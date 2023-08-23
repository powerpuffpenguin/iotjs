#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>
#include <iotjs/core/js.h>

// 爲棧頂的 n 個元素創建一個快照，存儲到 heap_stash["snapshot"][bucket][key] 中
// 如果 move 爲 true 則會將當前棧的的元素刪除，爲 false 則保留
void vm_snapshot_create(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_size_t n, duk_bool_t move);

// 將快照數組恢復到棧頂，如果快照不存在 require 爲 true 拋出異常，否則 不改變棧結構 並返回 false
duk_bool_t vm_snapshot_array(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot, duk_bool_t require);

// 如果快照存在，將存儲的快照恢復到棧頂 並且返回恢復元素數量
// 快照不存在，設置 require 爲 true 將拋出異常，否則返回 0
// del_snapshot 爲 true 則在恢復後刪除快照
// 通常只應該在異步回調中用於恢復棧
duk_size_t vm_snapshot_restore(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key, duk_bool_t del_snapshot, duk_bool_t require);

// 如果快照存在則刪除快照
void vm_snapshot_remove(duk_context *ctx, const char *bucket, duk_size_t sz_bucket, void *key);

typedef void (*vm_async_work)(void *args);
typedef struct
{
    vm_context_t *vm;
    void *in;
    void *out;
} vm_job_t;

vm_job_t *vm_new_job(duk_context *ctx, size_t in, size_t out);
duk_bool_t vm_run_job(vm_job_t *job, void (*work)(vm_job_t *job));
void vm_must_run_job(duk_context *ctx, vm_job_t *job, void (*work)(vm_job_t *job));
#endif