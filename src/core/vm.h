#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H

#include "duktape.h"

#define VM_ERROR_NEW_CONTEXT 1
#define VM_ERRNO_VAR(var, code) \
    if (var)                    \
    {                           \
        *var = code;            \
    }
typedef struct
{
    duk_context *ctx;
} vm_t;
#define VM_ERRNO(code) VM_ERRNO_VAR(errno, code)

const char *vm_error(int errno);
vm_t *vm_new(int *errno);

// ... -> ... result (if success, return value == 0)
// ... -> ... err (if failure, return value != 0)
duk_ret_t vm_main(vm_t *vm, const char *path);
void vm_delete(vm_t *vm);

#endif