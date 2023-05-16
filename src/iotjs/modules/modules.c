#include <iotjs/modules/modules.h>
#include <iotjs/core/vm.h>
__attribute((constructor)) void __iotjs_modules_init()
{
    vm_register("iotjs", native_iotjs_init);
    vm_register("iotjs/fs", native_iotjs_fs_init);
}