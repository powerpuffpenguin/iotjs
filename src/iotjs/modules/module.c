#include <iotjs/modules/module.h>
#include <iotjs/core/module.h>
void __iotjs_modules_init()
{
    vm_register_native("iotjs", native_iotjs_init);
    vm_register_native("iotjs/coroutine", native_iotjs_coroutine_init);
    vm_register_native("iotjs/encoding/hex", native_iotjs_encoding_hex_init);
    vm_register_native("iotjs/bytes", native_iotjs_bytes_init);
    vm_register_native("iotjs/mtd", native_iotjs_mtd_init);

    vm_register_native("iotjs/crypto/md5", native_iotjs_crypto_md4_init);
    vm_register_native("iotjs/crypto/md5", native_iotjs_crypto_md5_init);
    vm_register_native("iotjs/crypto/sha1", native_iotjs_crypto_sha1_init);
    vm_register_native("iotjs/crypto/sha224", native_iotjs_crypto_sha224_init);
    vm_register_native("iotjs/crypto/sha256", native_iotjs_crypto_sha256_init);
    vm_register_native("iotjs/crypto/sha384", native_iotjs_crypto_sha384_init);
    vm_register_native("iotjs/crypto/sha512", native_iotjs_crypto_sha512_init);
    vm_register_native("iotjs/crypto/sha512_224", native_iotjs_crypto_sha512_224_init);
    vm_register_native("iotjs/crypto/sha512_256", native_iotjs_crypto_sha512_256_init);

    // vm_register_native("iotjs/fs", native_iotjs_fs_init);
    vm_register_native("iotjs/net", native_iotjs_net_init);
    vm_register_native("iotjs/net/http", native_iotjs_net_http_init);
}