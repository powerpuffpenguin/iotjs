#include <iotjs/modules/module.h>
#include <iotjs/core/module.h>
__attribute((constructor)) void __iotjs_modules_init()
{
    vm_register_native("iotjs", native_iotjs_init);
    vm_register_native("iotjs/fs", native_iotjs_fs_init);
}