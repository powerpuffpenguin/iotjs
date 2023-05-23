#include <iotjs/core/c.h>
#include <iotjs/core/js.h>
#include <netinet/in.h>
typedef struct
{
    vm_context_t *vm;
    const char *name;
} dns_resolve_async_t;

duk_ret_t dns_resolve_finalizer(duk_context *ctx)
{

    duk_get_prop_lstring(ctx, 0, "ptr", 3);
    dns_resolve_async_t *p = duk_require_pointer(ctx, -1);
    if (!p || !p->vm)
    {
        return 0;
    }
    vm_context_t *vm = p->vm;
    if (vm->dns > 0)
    {
        vm->dns--;
    }
    if (!vm->dns && vm->esb)
    {
        evdns_base_free(vm->esb, 1);
        vm->esb = NULL;
    }

    IOTJS_FREE(p);
    return 0;
}
typedef struct
{
    int err;
    char type;
    int count;
    // int ttl;
    void *addresses;
} dns_resolve_handler_t;
duk_ret_t _vm_iotjs_nativa_dns_resolve_handler(duk_context *ctx)
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
void dns_resolve_handler(int err, char type, int count, int ttl, void *addresses, void *arg)
{
    dns_resolve_async_t *req = (dns_resolve_async_t *)arg;
    duk_context *ctx = req->vm->ctx;

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_ASYNC);
    duk_remove(ctx, -2);
    duk_push_pointer(ctx, req);
    duk_get_prop(ctx, -2);
    duk_push_pointer(ctx, req);
    duk_del_prop(ctx, -3);
    duk_remove(ctx, -2);

    duk_get_prop_lstring(ctx, -1, "completer", 9);

    dns_resolve_handler_t result = {
        .err = err,
        .type = type,
        .count = count,
        // .ttl = ttl,
        .addresses = addresses,
    };

    duk_push_c_function(ctx, _vm_iotjs_nativa_dns_resolve_handler, 1);
    duk_push_pointer(ctx, &result);
    if (duk_pcall(ctx, 1))
    {
        duk_push_lstring(ctx, "reject", 6);
        duk_swap_top(ctx, -2);
        duk_call_prop(ctx, -3, 1);
    }
    else
    {
        duk_push_lstring(ctx, "resolve", 7);
        duk_swap_top(ctx, -2);
        duk_call_prop(ctx, -3, 1);
    }
    duk_pop_3(ctx);
    // vm_dump_context_stdout(ctx);
}
void _vm_iotjs_nativa_dns_resolve_ip(duk_context *ctx, BOOL v6)
{
    const char *name = duk_require_string(ctx, 0);
    dns_resolve_async_t *p = vm_malloc_with_finalizer(ctx, sizeof(dns_resolve_async_t), dns_resolve_finalizer);
    p->name = name;
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "name", 4);

    vm_context_t *vm = vm_get_context_flags(ctx, VM_CONTEXT_FLAGS_ESB | VM_CONTEXT_FLAGS_COMPLETER);

    vm->dns++;
    p->vm = vm;

    duk_new(ctx, 0);
    duk_get_prop_lstring(ctx, -1, "promise", 7);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -3, "completer", 9);
    duk_swap_top(ctx, -2);

    // [promise, finalizer]
    if (v6)
    {
        evdns_base_resolve_ipv6(vm->esb, name, 0, dns_resolve_handler, p);
    }
    else
    {
        evdns_base_resolve_ipv4(vm->esb, name, 0, dns_resolve_handler, p);
    }

    duk_push_pointer(ctx, p);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_ASYNC);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -3);
    duk_put_prop(ctx, -3);

    duk_pop(ctx);
}
duk_ret_t _vm_iotjs_nativa_dns_resolve_ip4(duk_context *ctx)
{
    _vm_iotjs_nativa_dns_resolve_ip(ctx, FALSE);
    return 1;
}
duk_ret_t _vm_iotjs_nativa_dns_resolve_ip6(duk_context *ctx)
{
    _vm_iotjs_nativa_dns_resolve_ip(ctx, TRUE);
    return 1;
}
duk_ret_t _vm_iotjs_nativa_dns_ip_to_string(duk_context *ctx)
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
duk_ret_t _vm_iotjs_nativa_dns_parse_ip4(duk_context *ctx)
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
duk_ret_t _vm_iotjs_nativa_dns_parse_ip6(duk_context *ctx)
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
        duk_push_c_function(ctx, _vm_iotjs_nativa_dns_resolve_ip4, 1);
        duk_put_prop_lstring(ctx, -2, "resolveIP4", 10);
        duk_push_c_function(ctx, _vm_iotjs_nativa_dns_resolve_ip6, 1);
        duk_put_prop_lstring(ctx, -2, "resolveIP6", 10);
        duk_push_c_function(ctx, _vm_iotjs_nativa_dns_ip_to_string, 1);
        duk_put_prop_lstring(ctx, -2, "ipToString", 10);
        duk_push_c_function(ctx, _vm_iotjs_nativa_dns_parse_ip4, 1);
        duk_put_prop_lstring(ctx, -2, "parseIP4", 8);
        duk_push_c_function(ctx, _vm_iotjs_nativa_dns_parse_ip6, 1);
        duk_put_prop_lstring(ctx, -2, "parseIP6", 8);
    }
    duk_put_prop_lstring(ctx, -2, "dns", 3);
}