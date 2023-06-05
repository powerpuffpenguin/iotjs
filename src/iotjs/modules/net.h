#ifndef IOTJS_MODULES_NET_H
#define IOTJS_MODULES_NET_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/sniffer.h>

#ifndef EVENT__HAVE_OPENSSL
#define EVENT__HAVE_OPENSSL 1
#endif
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/dns.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <iotjs/core/js.h>
#include <iotjs/core/async.h>

// 包裝給 js 使用 的原始 tcp 連接
typedef struct
{
    vm_context_t *vm;
    struct event *timeout;
    struct bufferevent *bev;
    SSL_CTX *ssl_ctx;
    duk_uint32_t step;
} tcp_connection_t;
#endif