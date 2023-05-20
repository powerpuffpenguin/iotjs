#include <iotjs/modules/module.h>
#include <iotjs/core/module.h>
__attribute((constructor)) void __iotjs_modules_init()
{
    vm_register_native("iotjs", native_iotjs_init);
    vm_register_native("iotjs/encoding/hex", native_iotjs_encoding_hex_init);
    vm_register_native("iotjs/fs", native_iotjs_fs_init);
    vm_register_native("iotjs/net", native_iotjs_net_init);
}