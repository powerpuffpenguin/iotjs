#include <iotjs/core/configure.h>
#include <iotjs/core/c.h>
#include <iotjs/core/js.h>
#include <iotjs/core/async.h>
#include <netinet/in.h>

typedef struct
{
    vm_context_t *vm;
    const char *name;
} dns_resolve_t;
void dns_resolve_free(void *arg)
{
#ifdef VM_TRACE_FINALIZER
    puts("dns_resolve_free");
#endif
    dns_resolve_t *p = arg;
    if (!p || !p->vm)
    {
        return;
    }
    vm_free_dns(p->vm);
}
typedef struct
{
    int err;
    char type;
    int count;
    // int ttl;
    void *addresses;
} dns_resolve_handler_t;
static duk_ret_t nativa_dns_resolve_handler(duk_context *ctx)
{
    dns_resolve_handler_t *result = duk_require_pointer(ctx, 0);
    duk_pop(ctx);
    if (result->err)
    {
        duk_push_string(ctx, evdns_err_to_string(result->err));
        duk_throw(ctx);
    }
    char *p = result->addresses;
    switch (result->type)
    {
    case DNS_IPv4_A:
        duk_push_array(ctx);
        for (int i = 0; i < result->count; i++)
        {
            void *dst = duk_push_buffer(ctx, 4, 0);
            memcpy(dst, p, 4);
            duk_put_prop_index(ctx, -2, i);
            p += 4;
        }
        break;
    case DNS_IPv6_AAAA:
        duk_push_array(ctx);
        for (int i = 0; i < result->count; i++)
        {
            void *dst = duk_push_buffer(ctx, 16, 0);
            memcpy(dst, p, 16);
            duk_put_prop_index(ctx, -2, i);
            p += 16;
        }
        break;
    default:
        duk_push_string(ctx, "unknow type");
        duk_throw(ctx);
        break;
    }
    return 1;
}
static void dns_resolve_handler(int err, char type, int count, int ttl, void *addresses, void *arg)
{
    finalizer_t *finalizer = arg;
    dns_resolve_t *req = finalizer->p;
    duk_context *ctx = req->vm->ctx;

    dns_resolve_handler_t result = {
        .err = err,
        .type = type,
        .count = count,
        // .ttl = ttl,
        .addresses = addresses,
    };

    duk_push_c_lightfunc(ctx, nativa_dns_resolve_handler, 1, 1, 0);
    duk_push_pointer(ctx, &result);
    if (duk_pcall(ctx, 1))
    {
        vm_async_reject_get(ctx, finalizer);
    }
    else
    {
        vm_async_resolve_get(ctx, finalizer);
    }
    duk_get_prop_lstring(ctx, -1, "args", 4);
    vm_finalizer_free(ctx, -1, dns_resolve_free);
    duk_pop_2(ctx);
}
static void nativa_resolve_ip(duk_context *ctx, BOOL v6)
{
    const char *name = duk_require_string(ctx, 0);
    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(dns_resolve_t));
    finalizer->free = dns_resolve_free;
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "name", 4);

    // [finalizer]
    dns_resolve_t *p = finalizer->p;
    p->name = name;

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB | VM_CONTEXT_FLAGS_COMPLETER);
    vm->dns++;
    p->vm = vm;

    // [finalizer, complater]
    if (v6)
    {
        struct evdns_request *req = evdns_base_resolve_ipv6(vm->esb, name, 0, dns_resolve_handler, finalizer);
        if (!req)
        {
            vm_finalizer_free(ctx, -3, dns_resolve_free);
            duk_error(ctx, DUK_ERR_ERROR, "evdns_base_resolve_ipv6 error");
        }
    }
    else
    {
        struct evdns_request *req = evdns_base_resolve_ipv4(vm->esb, name, 0, dns_resolve_handler, finalizer);
        if (!req)
        {
            vm_finalizer_free(ctx, -3, dns_resolve_free);
            duk_error(ctx, DUK_ERR_ERROR, "evdns_base_resolve_ipv4 error");
        }
    }

    vm_async_completer_args(ctx, finalizer);
}
static duk_ret_t nativa_resolve_ip4(duk_context *ctx)
{
    nativa_resolve_ip(ctx, FALSE);
    return 1;
}
static duk_ret_t nativa_resolve_ip6(duk_context *ctx)
{
    nativa_resolve_ip(ctx, TRUE);
    return 1;
}
static duk_ret_t nativa_ip_to_string(duk_context *ctx)
{
    duk_size_t out_size;
    char *p = duk_require_buffer_data(ctx, 0, &out_size);
    duk_pop(ctx);
    switch (out_size)
    {
    case 4:
    {
        char ip[16];
        evutil_inet_ntop(AF_INET, p, ip, 16);
        duk_push_string(ctx, ip);
    }
    break;
    case 16:
    {
        char ip[40] = {0};
        evutil_inet_ntop(AF_INET6, p, ip, 40);
        duk_push_string(ctx, ip);
    }
    break;
    default:
        duk_push_lstring(ctx, "buffer not a ip4 or ip6", 23);
        duk_throw(ctx);
        break;
    }
    return 1;
}
static duk_ret_t nativa_parse_ip4(duk_context *ctx)
{
    const char *s = duk_require_string(ctx, 0);
    struct in_addr *addr = malloc(sizeof(struct in_addr));
    if (!evutil_inet_pton(AF_INET, s, addr))
    {
        duk_push_lstring(ctx, "not a ipv4: ", 12);
        duk_swap_top(ctx, -2);
        duk_concat(ctx, 2);
        duk_throw(ctx);
    }
    duk_pop(ctx);
    void *dst = duk_push_buffer(ctx, 4, 0);
    memcpy(dst, &addr->s_addr, 4);
    return 1;
}
static duk_ret_t nativa_parse_ip6(duk_context *ctx)
{
    const char *s = duk_require_string(ctx, 0);
    struct in6_addr *addr = malloc(sizeof(struct in6_addr));
    if (!evutil_inet_pton(AF_INET6, s, addr))
    {
        duk_push_lstring(ctx, "not a ipv6: ", 12);
        duk_swap_top(ctx, -2);
        duk_concat(ctx, 2);
        duk_throw(ctx);
    }
    duk_pop(ctx);
    void *dst = duk_push_buffer(ctx, 16, 0);
    memcpy(dst, &addr->__in6_u, 16);
    return 1;
}
void _vm_init_c(duk_context *ctx)
{
    // dns
    duk_push_object(ctx);
    {
        duk_push_c_function(ctx, nativa_resolve_ip4, 1);
        duk_put_prop_lstring(ctx, -2, "resolveIP4", 10);
        duk_push_c_function(ctx, nativa_resolve_ip6, 1);
        duk_put_prop_lstring(ctx, -2, "resolveIP6", 10);
        duk_push_c_function(ctx, nativa_ip_to_string, 1);
        duk_put_prop_lstring(ctx, -2, "ipToString", 10);
        duk_push_c_function(ctx, nativa_parse_ip4, 1);
        duk_put_prop_lstring(ctx, -2, "parseIP4", 8);
        duk_push_c_function(ctx, nativa_parse_ip6, 1);
        duk_put_prop_lstring(ctx, -2, "parseIP6", 8);
    }
    duk_put_prop_lstring(ctx, -2, "dns", 3);
}