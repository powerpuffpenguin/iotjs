#include <iotjs/core/vm.h>
#include <iotjs/core/strings.h>
#include <iotjs/core/path.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "duk_module_node.h"
#include "duk_console.h"

duk_ret_t cb_resolve_module(duk_context *ctx);
duk_ret_t cb_load_module(duk_context *ctx);

const char *vm_error(int err)
{
    switch (err)
    {
    case VM_ERROR_NEW_CONTEXT:
        return "duk_create_heap error";
    }
    return "Unknow Error";
}
vm_t *vm_new(int *err)
{
    duk_context *ctx = duk_create_heap_default();
    if (!ctx)
    {
        VM_ERR(VM_ERROR_NEW_CONTEXT);
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
char *readFile(const char *filename, size_t *len, duk_ret_t *err)
{
    FILE *f;
    f = fopen(filename, "r");
    if (!f)
    {
        *err = DUK_EXEC_ERROR;
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    char *buffer = (char *)malloc(*len);
    fseek(f, 0, SEEK_SET);
    fread(buffer, 1, *len, f);
    fclose(f);
    return buffer;
}
void dump_context_stdout(duk_context *ctx)
{
    duk_push_context_dump(ctx);
    fprintf(stdout, "%s\n", duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
}
duk_ret_t vm_read_file(duk_context *ctx, const char *path)
{
    FILE *f;
    f = fopen(path, "r");
    if (!f)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "fopen(%d) %s: %s", errno, strerror(errno), path);
        return DUK_EXEC_ERROR;
    }
    if (!fseek(f, 0, SEEK_END))
    {
        int err = errno;
        fclose(f);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "fseek(%d) %s: %s", err, strerror(err), path);
        return DUK_EXEC_ERROR;
    }
    long n = ftell(f);
    if (n < 0)
    {
        int err = errno;
        fclose(f);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "ftell(%d) %s: %s", err, strerror(err), path);
        return DUK_EXEC_ERROR;
    }
    if (!fseek(f, 0, SEEK_SET))
    {
        int err = errno;
        fclose(f);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "fseek(%d) %s: %s", err, strerror(err), path);
        return DUK_EXEC_ERROR;
    }
    
    dump_context_stdout(ctx);
    fclose(f);
    return DUK_EXEC_SUCCESS;
}
duk_ret_t vm_main(vm_t *vm, const char *path)
{
    // duk_ret_t err = DUK_EXEC_SUCCESS;
    // size_t len;
    // char *s = readFile(path, &len, &err);
    // duk_push_lstring(vm->ctx, s, len);
    duk_ret_t err = vm_read_file(vm->ctx, path);
    if (err)
    {
        return err;
    }
    return duk_module_node_peval_main(vm->ctx, path);
}
void vm_delete(vm_t *vm)
{
    duk_destroy_heap(vm->ctx);
    free(vm);
}

duk_ret_t cb_resolve_module(duk_context *ctx)
{
    /*
     *  Entry stack: [ requested_id parent_id ]
     */

    size_t requested_len;
    const char *requested_id = duk_get_lstring(ctx, 0, &requested_len);
    if (!requested_len)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "require expects a valid module id");
        duk_throw(ctx);
    }
    size_t parent_len;
    const char *parent_id = duk_get_lstring(ctx, 1, &parent_len); /* calling module */

    printf("pid=%s id=%s \n", parent_id, requested_id);

    /* Arrive at the canonical module ID somehow. */
    string_t resolved_id;
    if (parent_len)
    {
        size_t n = parent_len + 1 + requested_len;
        resolved_id = strings_make(n);
        if (IOTJS_REFERENCE_INVALID(resolved_id))
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "require strings_make() error");
            duk_throw(ctx);
        }
        char *p = IOTJS_REFERENCE_PTR(resolved_id);
        memcpy(p, parent_id, parent_len);
        p[parent_len] = '/';
        memcpy(p + parent_len + 1, requested_id, requested_len);

        resolved_id = path_clean(&resolved_id, TRUE);
        if (IOTJS_REFERENCE_INVALID(resolved_id))
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "require path_clean(resolved_id) error");
            duk_throw(ctx);
        }
    }
    else
    {
        resolved_id = strings_from_str(requested_id, requested_len);
        if (IOTJS_REFERENCE_INVALID(resolved_id))
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "require strings_from_str(requested_id) error");
            duk_throw(ctx);
        }
        resolved_id = path_clean(&resolved_id, TRUE);
        if (IOTJS_REFERENCE_INVALID(resolved_id))
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "require path_clean(resolved_id) error");
            duk_throw(ctx);
        }
    }
    duk_push_lstring(ctx, IOTJS_REFERENCE_PTR(resolved_id), resolved_id.len);
    strings_decrement(&resolved_id);
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