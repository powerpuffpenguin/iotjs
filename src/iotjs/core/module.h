#ifndef IOTJS_CORE_MODULE_H
#define IOTJS_CORE_MODULE_H
#include <duktape.h>

// 向系統註冊一個本地模塊
void vm_register_native(const char *name, duk_c_function init);

void _vm_init_native(duk_context *ctx);

// ... => ... path
void vm_path(duk_context *ctx);
#endif