#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H
#include <duktape.h>
#include <event2/event.h>

#define VM_VERSION "v0.0.1"

typedef struct event_base event_base_t;

// 加載 最初的 main 腳本
duk_ret_t vm_main(duk_context *ctx, const char *path);

// 成功返回 事件循環 ... => ...
// 失敗返回 NULL    ... => ... error
event_base_t *vm_event_base(duk_context *ctx);
void vm_dump_context_stdout(duk_context *ctx);

typedef struct
{
    const char *name;
    duk_c_function init;
} vm_native_module_t;
void vm_register(const char *name, duk_c_function init);

// #define VM_DEBUG_MODULE_LOAD 1

#endif