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
void _vm_init_context(duk_context *ctx);

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
    void *arg;
    // 在工作線程執行的異步代碼
    void (*work)(struct vm_async_job *job, void *arg);
} vm_async_job_t;
typedef void (*vm_async_job_function)(vm_async_job_t *job, void *arg);
// 創建一個異步工作
// duk_throw ... => ... Promise, job:{ptr,completer}
vm_async_job_t *vm_new_async_job(duk_context *ctx, vm_async_job_function work, size_t sz_arg);
// 執行異步工作
// duk_throw ... job => ...
void vm_execute_async_job(duk_context *ctx);

#endif