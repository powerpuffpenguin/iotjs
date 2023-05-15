#ifndef IOTJS_CORE_JS_H
#define IOTJS_CORE_JS_H

#include <iotjs/core/defines.h>
#include <duktape.h>
#include <event2/event.h>

#define VM_VERSION "v0.0.1"

typedef struct event_base event_base_t;

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