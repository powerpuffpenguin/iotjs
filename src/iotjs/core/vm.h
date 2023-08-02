#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H
#include <duktape.h>
#include <event2/event.h>

// 初始化系統，這是在創建 duk 後應該調用的第一個函數，它爲 duk 實現了各種擴展功能
duk_int_t vm_init(duk_context *ctx, int argc, char *argv[]);
// 加載 最初的 main 腳本
duk_int_t vm_main(duk_context *ctx, const char *path);
// 加載 最初的 main 腳本
// ... source => ... return
duk_int_t vm_main_source(duk_context *ctx, const char *path);

// 執行 主循環
duk_int_t vm_loop(duk_context *ctx);
// #define VM_DEBUG_MODULE_LOAD 1
struct event_base *vm_base(duk_context *ctx);

#endif