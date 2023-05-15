#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H
#include <iotjs/core/js.h>

// 加載 最初的 main 腳本
duk_ret_t vm_main(duk_context *ctx, const char *path);

// #define VM_DEBUG_MODULE_LOAD 1

#endif