#include <iotjs/modules/module.h>
#include <iotjs/core/binary.h>
#include <tomcrypt.h>
void vm_init_core()
{
    register_cipher(&aes_desc);

    __iotjs_binary_init();
    __iotjs_modules_init();
}