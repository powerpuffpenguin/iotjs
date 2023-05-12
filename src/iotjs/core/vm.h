#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H

#include "duktape.h"
#define VM_VERSION "v0.0.1"

duk_ret_t vm_main(duk_context *vm, const char *path);
void vm_dump_context_stdout(duk_context *ctx);

typedef struct
{
    const char *name;
    duk_c_function init;
} vm_native_module_t;
void vm_register(const char *name, duk_c_function init);

// #define VM_DEBUG_MODULE_LOAD 1

#endif