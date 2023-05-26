#ifndef OPENSSL_COMPAT_H
#define OPENSSL_COMPAT_H

#include <wolfssl/options.h>
#include <wolfssl/openssl/bio.h>
#include "util-internal.h"

#define BIO_get_init(b) (b)->init
#define BIO_number_written(v) 0
#define BIO_number_read(v) 0
#define SSL WOLFSSL
#define X509_STORE_set_default_paths wolfSSL_X509_STORE_set_default_paths

#endif /* OPENSSL_COMPAT_H */
