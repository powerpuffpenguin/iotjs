#ifndef IOTJS_MODULES_CRYPTO_HASH_H
#define IOTJS_MODULES_CRYPTO_HASH_H

int iotjs_module_crypto_sha1(const unsigned char *in, unsigned long inlen, unsigned char *hash);
int iotjs_module_crypto_base64_encode(const unsigned char *in, unsigned long len, unsigned char *out, unsigned long *outlen);
#endif
