#include <iotjs/modules/module.h>
#include <iotjs/core/binary.h>
void vm_init_core()
{
    __iotjs_binary_init();
    __iotjs_modules_init();
}