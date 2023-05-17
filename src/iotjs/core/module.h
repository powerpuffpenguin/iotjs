#ifndef IOTJS_CORE_MODULE_H
#define IOTJS_CORE_MODULE_H
#include <duktape.h>
typedef struct
{
    const char *name;
    duk_c_function init;
} vm_native_module_t;
void vm_register(const char *name, duk_c_function init);

void _vm_init_native(duk_context *ctx);
#endif