#ifndef IOTJS_CORE_JS_H
#define IOTJS_CORE_JS_H

#include <iotjs/core/defines.h>
#include <duktape.h>
#include <event2/event.h>
#include <thpool.h>
#include <pthread.h>

#define VM_VERSION "v0.0.1"

typedef struct event_base event_base_t;
typedef struct event event_t;
typedef struct timeval time_value_t;

typedef struct
{
    // 最外層的 context 用於異步完成時調用 js 回調
    duk_context *ctx;
    // 用於創建 event 以便通知異步完成
    event_base_t *eb;
    // 線程池用於 將耗時操作提交到工作線程
    threadpool threads;
    // 同步
    pthread_mutex_t mutex;
} vm_context_t;
// 由系統調用，初始化 vm_context_t
void _vm_init_context(duk_context *ctx, duk_context *main);

// 打印js棧到 stdout 以供調試
void vm_dump_context_stdout(duk_context *ctx);

// 返回運行環境
// duk_throw ... => ...
vm_context_t *vm_get_context(duk_context *ctx);

// 動態申請一塊內存，將它和 finalizer 函數關聯，以便 js 可以自動關聯它
// duk_throw ... => ... obj{ptr}
void *vm_malloc_with_finalizer(duk_context *ctx, size_t sz, duk_c_function func);

// 返回 finalizer 關聯的 c 指針
// duk_throw ... obj{ptr} => ...
void *vm_get_finalizer_ptr(duk_context *ctx);

// 存儲一個異步工作
typedef struct vm_async_job
{
    // 系統環境
    vm_context_t *vm;
    // 通知異步完成的事件
    event_t *ev;
    // 傳遞給工作線程的可選參數
    void *in;
    // 存儲工作完成後返回的結果
    void *out;
    // 執行結果的錯誤代碼，0 表示成功
    int err;
    // 在工作線程執行的異步代碼
    void (*work)(struct vm_async_job *job, void *in);
    // 異步完成後自動調用此回調函數，如果不設置則不進行調用
    // [job]
    duk_c_function complete;
} vm_async_job_t;
typedef void (*vm_async_job_function)(vm_async_job_t *job, void *in);
// 創建一個異步工作
// duk_throw ... => ... job:{ptr,completer}
vm_async_job_t *vm_new_async_job(duk_context *ctx, vm_async_job_function work, size_t sz_in, size_t sz_out);
// 執行異步工作
// duk_throw ... job => ... promise
void vm_execute_async_job(duk_context *ctx, vm_async_job_t *job);
// 通知異步工作完成，這通常在工作線程中調用用於通知事件系統 處理完成的異步結果
void vm_complete_async_job(vm_async_job_t *job);
// 返回 job 指針
// duk_throw ... job => ... job
vm_async_job_t *vm_get_async_job(duk_context *ctx);

// 通知 js 異步錯誤
// duk_throw ... job ... err => ... job ...
void vm_reject_async_job(duk_context *ctx, duk_idx_t i);
// 通知 js 異步成功
// duk_throw ... job ... err => ... job ...
void vm_resolve_async_job(duk_context *ctx, duk_idx_t i);
#endif