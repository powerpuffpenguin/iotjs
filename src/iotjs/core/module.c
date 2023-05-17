#include <iotjs/core/module.h>
#include <iotjs/core/defines.h>
typedef struct
{
    const char *name;
    duk_c_function init;
} vm_native_module_t;
typedef struct
{
    size_t len;
    size_t cap;
    vm_native_module_t *modules;
} vm_native_modules_t;
vm_native_modules_t _vm_default_modules = {
    .len = 0,
    .cap = 0,
    .modules = NULL,
};
void vm_register_native(const char *name, duk_c_function init)
{
    vm_native_modules_t *modules = &_vm_default_modules;
    for (size_t i = 0; i < modules->len; i++)
    {
        if (!strcmp(name, modules->modules[i].name))
        {
            modules->modules[i].init = init;
            return;
        }
    }

    if (modules->len + 1 > modules->cap)
    {
        size_t cap = modules->cap;
        if (cap == 0)
        {
            cap = 16;
        }
        else if (cap < 128)
        {
            cap *= 2;
        }
        else
        {
            cap += 16;
        }
        vm_native_module_t *p = (vm_native_module_t *)malloc(sizeof(vm_native_module_t) * cap);
        if (modules->len)
        {
            memcpy(p, modules->modules, sizeof(vm_native_module_t) * modules->len);
            free(modules->modules);
        }
        modules->modules = p;
        modules->cap = cap;
    }
    int i = modules->len;
    modules->modules[i].name = name;
    modules->modules[i].init = init;
    modules->len++;
}
void _vm_init_native(duk_context *ctx)
{
    duk_require_stack(ctx, 2);
    duk_push_object(ctx);
    vm_native_modules_t *modules = &_vm_default_modules;
    for (size_t i = 0; i < modules->len; i++)
    {
        if (modules->modules[i].init && modules->modules[i].name)
        {
            duk_push_c_function(ctx, modules->modules[i].init, 3);
            duk_put_prop_string(ctx, -2, modules->modules[i].name);
        }
    }
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_NATIVE);
}
void vm_path(duk_context *ctx)
{
    duk_require_stack(ctx, 4);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_C);
    duk_get_prop_lstring(ctx, -1, "path", 4);
    duk_swap_top(ctx, -3);
    duk_pop_2(ctx);
}