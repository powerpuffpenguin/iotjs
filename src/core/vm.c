#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include "duk_module_node.h"
#include "duk_console.h"

duk_ret_t cb_resolve_module(duk_context *ctx);
duk_ret_t cb_load_module(duk_context *ctx);

const char *vm_error(int errno)
{
    switch (errno)
    {
    case VM_ERROR_NEW_CONTEXT:
        return "duk_create_heap error";
    }
    return "Unknow Error";
}
vm_t *vm_new(int *errno)
{
    duk_context *ctx = duk_create_heap_default();
    if (!ctx)
    {
        VM_ERRNO(VM_ERROR_NEW_CONTEXT);
        return NULL;
    }
    duk_console_init(ctx, 0);

    duk_push_object(ctx);
    duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "resolve");
    duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "load");
    duk_module_node_init(ctx);

    vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
    vm->ctx = ctx;
    return vm;
}
duk_ret_t vm_main(vm_t *vm, const char *path)
{
    duk_ret_t errno;
    duk_push_global_object(vm->ctx);
    {
        duk_get_prop_string(vm->ctx, -1, "require");
        duk_push_string(vm->ctx, path);

        errno = duk_pcall(vm->ctx, 1);
        duk_swap_top(vm->ctx, -2);
    }
    duk_pop(vm->ctx);
    return errno;
}
void vm_delete(vm_t *vm)
{
    duk_destroy_heap(vm->ctx);
    free(vm);
}

duk_ret_t cb_resolve_module(duk_context *ctx)
{
    puts("cb_resolve_module");
    /*
     *  Entry stack: [ requested_id parent_id ]
     */

    const char *requested_id = duk_get_string(ctx, 0);
    const char *parent_id = duk_get_string(ctx, 1); /* calling module */
    const char *resolved_id;

    /* Arrive at the canonical module ID somehow. */
    printf("id=%s pid=%s\n", requested_id, parent_id);

    duk_push_string(ctx, resolved_id);
    return 1; /*nrets*/
}
duk_ret_t cb_load_module(duk_context *ctx)
{
    puts("cb_load_module");
    /*
     *  Entry stack: [ resolved_id exports module ]
     */

    /* Arrive at the JS source code for the module somehow. */

    // duk_push_string(ctx, module_source);
    // return 1; /*nrets*/
    return 0;
}