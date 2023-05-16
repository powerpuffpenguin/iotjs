#ifndef IOTJS_CORE_JS_H
#define IOTJS_CORE_JS_H

#include <iotjs/core/defines.h>
#include <duktape.h>
#include <event2/event.h>
#include <thpool.h>

#define VM_VERSION "v0.0.1"

typedef struct event_base event_base_t;
typedef struct event event_t;

typedef struct
{
    // 最外層的 context 用於異步完成時調用 js 回調
    duk_context *ctx;
    // 用於創建 event 以便通知異步完成
    struct event_base *eb;
    // 線程池用於 將耗時操作提交到工作線程
    threadpool threads;
} vm_context_t;

void vm_init_context(duk_context *ctx);

// 打印js棧到 stdout 以供調試
void vm_dump_context_stdout(duk_context *ctx);

// duk_throw
// 成功返回 事件循環 ... => ...
event_base_t *vm_event_base(duk_context *ctx);
// duk_throw
// 成功返回 主 contexgt ... => ...
duk_context *vm_context(duk_context *ctx);

typedef struct
{
    const char *name;
    duk_c_function init;
} vm_native_module_t;
void vm_register(const char *name, duk_c_function init);

#endif