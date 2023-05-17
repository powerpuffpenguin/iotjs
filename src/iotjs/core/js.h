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

// duk_throw
// ... => ...
vm_context_t *vm_get_context(duk_context *ctx);

#endif