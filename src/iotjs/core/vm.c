#include <iotjs/core/vm.h>
#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <iotjs/core/timer.h>
#include <iotjs/core/c.h>
#include <iotjs/core/js/hook.h>
#include <iotjs/core/js/path.h>
#include <iotjs/core/js/module.h>
#include <iotjs/core/es6-shim.h>

#include <duk_module_node.h>
#include <duk_console.h>

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
duk_ret_t _vm_native_getcwd(duk_context *ctx);
void _native_vm_init_stash(duk_context *ctx, duk_context *main);
void _native_vm_init_global(duk_context *ctx);
void _native_vm_read_text(duk_context *ctx, const char *path)
{
    struct stat fsstat;
    if (stat(path, &fsstat))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "stat error(%d) %s: %s", errno, strerror(errno), path);
        duk_throw(ctx);
    }
    else if (!S_ISREG(fsstat.st_mode))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "not a  regular file: %s", path);
        duk_throw(ctx);
    }

    FILE *f;
    f = fopen(path, "r");
    if (!f)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "fopen(%d) %s: %s", errno, strerror(errno), path);
        duk_throw(ctx);
    }

    void *buffer = duk_push_buffer(ctx, fsstat.st_size, 0);
    size_t readed = fread(buffer, 1, fsstat.st_size, f);
    if (readed != fsstat.st_size)
    {
        int err = errno;
        fclose(f);
        duk_push_error_object(ctx, DUK_ERR_ERROR, "fread(%d) %s: %s", err, strerror(err), path);
        duk_throw(ctx);
    }
    fclose(f);
    duk_buffer_to_string(ctx, -1);
}
duk_ret_t _native_vm_read_utf8(duk_context *ctx)
{
    _native_vm_read_text(ctx, duk_require_string(ctx, 0));
    return 1;
}
duk_ret_t _native_vm_resolve_module(duk_context *ctx)
{
    duk_require_stack_top(ctx, 9);
    /*
     *  Entry stack: [ requested_id parent_id ]
     */
#ifdef VM_DEBUG_MODULE_LOAD
    puts("--- cb_resolve_module");
    vm_dump_context_stdout(ctx);
#endif
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_NATIVE);
    duk_swap_top(ctx, -2);

    // [ requested_id, parent_id, native, stash]
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // [ requested_id, parent_id, native, _iotjs]
    duk_get_prop_lstring(ctx, -1, "resolve_module", 14);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, 0);

// [ resolve, parent_id, native, requested_id]
// vm_dump_context_stdout(ctx);
#ifdef VM_DEBUG_MODULE_LOAD
    duk_push_boolean(ctx, 1);
    duk_call(ctx, 4);
#else
    duk_call(ctx, 3);
#endif
    return 1; /*nrets*/
}
duk_ret_t _native_vm_load_module(duk_context *ctx)
{
    duk_require_stack_top(ctx, 6);
    /*
     *  Entry stack: [ resolved_id exports module ]
     */
#ifdef VM_DEBUG_MODULE_LOAD
    puts("--- cb_load_module");
    vm_dump_context_stdout(ctx);
#endif

    /* Arrive at the JS source code for the module somehow. */
    size_t len;
    const char *id = duk_get_lstring(ctx, 0, &len);
    if (len == 0)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "resolved_id invalid");
        duk_throw(ctx);
    }
    if (len > 1 && id[0] == '/')
    {
        id = duk_get_string(ctx, 0);
        _native_vm_read_text(ctx, id);
        return 1;
    }
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "native");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_dup(ctx, 0);
    duk_get_prop(ctx, -2);
    if (duk_is_c_function(ctx, -1))
    {
        duk_c_function f = duk_get_c_function(ctx, -1);
        duk_pop_2(ctx);
        return f(ctx);
    }
    duk_pop_2(ctx);
    duk_push_error_object(ctx, DUK_ERR_ERROR, "native module not exists");
    duk_throw(ctx);
    return 0;
}

duk_ret_t _native_vm_init(duk_context *ctx)
{
    // [argv, ctx]
    duk_context *main = duk_require_pointer(ctx, -1);
    duk_pop(ctx);

    duk_require_stack(ctx, 10);
    // [argv]
    duk_push_object(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "argv", 4);
    duk_dup_top(ctx);
    duk_push_global_object(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_IOTJS);
    duk_pop(ctx);

    // [iotjs]
    duk_push_heap_stash(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_IOTJS);
    _native_vm_init_stash(ctx, main);
    duk_pop(ctx);

    // []
    duk_push_global_object(ctx);
    _native_vm_init_global(ctx);
    duk_pop(ctx);

    duk_console_init(ctx, 0);

    duk_push_object(ctx);
    duk_push_c_function(ctx, _native_vm_resolve_module, 2);
    duk_put_prop_lstring(ctx, -2, "resolve", 7);
    duk_push_c_function(ctx, _native_vm_load_module, 3);
    duk_put_prop_lstring(ctx, -2, "load", 4);
    duk_module_node_init(ctx);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    _vm_init_c(ctx);
    duk_pop(ctx);
    return 0;
}

duk_int_t vm_init(duk_context *ctx, int argc, char *argv[])
{
    if (!duk_check_stack_top(ctx, 5))
    {
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "duk_check_stack_top error");
        return DUK_EXEC_ERROR;
    }
    duk_push_c_function(ctx, _native_vm_init, 2);
    duk_push_array(ctx);
    for (int i = 0; i < argc; i++)
    {
        duk_push_string(ctx, argv[i]);
        duk_put_prop_index(ctx, -2, i);
    }
    if (argc < 2)
    {
        duk_push_string(ctx, "main.js");
        duk_put_prop_index(ctx, -2, 1);
    }
    duk_push_pointer(ctx, ctx);
    duk_int_t err = duk_pcall(ctx, 2);
    if (!err)
    {
        duk_pop(ctx);
    }
    return err;
}
duk_ret_t _native_vm_main(duk_context *ctx)
{
    const char *path = duk_get_string(ctx, 0);
    if (path[0] != '/')
    {
        _vm_native_getcwd(ctx);
        duk_swap_top(ctx, -2);
        duk_push_lstring(ctx, "/", 1);
        duk_swap_top(ctx, -2);
        duk_concat(ctx, 3);
        path = duk_get_string(ctx, 0);
    }
    _native_vm_read_text(ctx, path);
    duk_swap_top(ctx, 0);
    if (duk_module_node_peval_main(ctx, path))
    {
        duk_throw(ctx);
    }
    return 1;
}
duk_ret_t _native_vm_main_source(duk_context *ctx)
{
    const char *path = duk_get_string(ctx, 1);
    if (path[0] != '/')
    {
        _vm_native_getcwd(ctx);
        duk_swap_top(ctx, -2);
        duk_push_lstring(ctx, "/", 1);
        duk_swap_top(ctx, -2);
        duk_concat(ctx, 3);
        path = duk_get_string(ctx, 0);
    }
    duk_pop(ctx);
    if (duk_module_node_peval_main(ctx, path))
    {
        duk_throw(ctx);
    }
    return 1;
}
duk_int_t vm_main(duk_context *ctx, const char *path)
{
    if (!duk_check_stack_top(ctx, 2))
    {
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "duk_check_stack_top error");
        return DUK_EXEC_ERROR;
    }
    duk_push_c_function(ctx, _native_vm_main, 1);
    duk_push_string(ctx, path);
    return duk_pcall(ctx, 1);
}
duk_int_t vm_main_source(duk_context *ctx, const char *path)
{
    if (!duk_check_stack_top(ctx, 2))
    {
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "duk_check_stack_top error");
        return DUK_EXEC_ERROR;
    }
    duk_push_c_function(ctx, _native_vm_main_source, 2);
    duk_swap_top(ctx, -2);
    duk_push_string(ctx, path);
    return duk_pcall(ctx, 2);
}
duk_ret_t _native_vm_loop(duk_context *ctx)
{
    vm_context_t *vm = (vm_context_t *)vm_get_context(ctx);
    int err = event_base_dispatch(vm->eb);
    if (err < 0)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_base_dispatch error");
        duk_throw(ctx);
    }
    return 0;
}
duk_int_t vm_loop(duk_context *ctx)
{
    if (!duk_check_stack_top(ctx, 2))
    {
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "duk_check_stack_top error");
        return DUK_EXEC_ERROR;
    }
    duk_push_c_function(ctx, _native_vm_loop, 0);
    return duk_pcall(ctx, 0);
}
duk_ret_t _native_vm_c_stat(duk_context *ctx)
{
    const char *path = duk_get_string(ctx, 0);
    struct stat fsstat;
    if (stat(path, &fsstat))
    {
        if (errno != ENOENT)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "stat error(%d): %s", errno, strerror(errno));
            duk_throw(ctx);
        }
        return 0;
    }
    duk_pop(ctx);
    duk_push_object(ctx);

    duk_uint_t mode = 0;
    if (S_ISDIR(fsstat.st_mode))
    {
        mode |= 0x1;
    }
    else if (S_ISREG(fsstat.st_mode))
    {
        mode |= 0x2;
    }
    duk_push_uint(ctx, mode);
    duk_put_prop_string(ctx, -2, "mode");
    return 1;
}

duk_ret_t _vm_native_getcwd(duk_context *ctx)
{
    char path[MAXPATHLEN];
    if (!getcwd(path, MAXPATHLEN))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "getcwd error(%d): %s", errno, strerror(errno));
        duk_throw(ctx);
    }
    duk_push_string(ctx, path);
    return 1;
}
duk_ret_t _vm_native_exit(duk_context *ctx)
{
    if (duk_is_number(ctx, -1))
    {
        exit(duk_get_int(ctx, -1));
    }
    else
    {
        exit(-1);
    }
    return 0;
}
duk_ret_t _vm_native_stat_module(duk_context *ctx)
{
    const char *path = duk_get_string(ctx, 0);
    struct stat fsstat;
    if (stat(path, &fsstat))
    {
        if (errno != ENOENT)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "stat error(%d): %s", errno, strerror(errno));
            duk_throw(ctx);
        }
        return 0;
    }
    duk_pop(ctx);
    duk_push_object(ctx);

    duk_uint_t mode = 0;
    if (S_ISDIR(fsstat.st_mode))
    {
        mode |= 0x1;
    }
    else if (S_ISREG(fsstat.st_mode))
    {
        mode |= 0x2;
    }
    duk_push_uint(ctx, mode);
    duk_put_prop_string(ctx, -2, "mode");
    return 1;
}
void _native_vm_init_stash(duk_context *ctx, duk_context *main)
{
    // [stash]
    duk_require_stack(ctx, 10);
    // private
    {
        duk_eval_lstring(ctx, (char *)js_iotjs_core_js_hook_min_js, js_iotjs_core_js_hook_min_js_len);
        duk_dup_top(ctx);
        duk_dup_top(ctx);
        duk_put_prop_lstring(ctx, -4, VM_STASH_KEY_PRIVATE);

        // 導入 path 用於解析路徑
        duk_push_object(ctx);
        duk_eval_lstring(ctx, (const char *)js_iotjs_core_js_path_min_js, js_iotjs_core_js_path_min_js_len);
        duk_swap_top(ctx, -3);
        duk_call(ctx, 2);
        duk_put_prop_lstring(ctx, -2, "path", 4);

        // [stash, _iotjs]
        duk_push_c_function(ctx, _native_vm_read_utf8, 1);
        duk_put_prop_lstring(ctx, -2, "read_text", 9);
        duk_push_c_function(ctx, _vm_native_stat_module, 1);
        duk_put_prop_lstring(ctx, -2, "stat_module", 11);
        duk_push_c_function(ctx, _vm_native_getcwd, 0);
        duk_put_prop_lstring(ctx, -2, "getcwd", 6);
        duk_push_c_function(ctx, _vm_native_exit, 1);
        duk_put_prop_lstring(ctx, -2, "exit", 4);

        duk_eval_lstring(ctx, (const char *)js_iotjs_core_js_module_min_js, js_iotjs_core_js_module_min_js_len);
        duk_dup(ctx, -2);
        duk_dup_top(ctx);
        duk_call(ctx, 2);

        duk_pop_2(ctx);
    }

    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_TIMEOUT);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_INTERVAL);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_JOBS);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_ASYNC);

    _vm_init_context(ctx, main);
    _vm_init_native(ctx);
}
static duk_ret_t _native_gc(duk_context *ctx)
{
    duk_gc(ctx, 0);
    return 0;
}
static duk_ret_t _native_nameserver(duk_context *ctx)
{
    duk_idx_t n = duk_get_top(ctx);
    if (n > 0)
    {
        if (n > 1)
        {
            duk_pop_n(ctx, n - 1);
        }
        duk_require_string(ctx, -1);
        duk_push_heap_stash(ctx);
        duk_swap_top(ctx, -2);
        duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_NAMESERVER);
        return 0;
    }
    else
    {
        duk_push_heap_stash(ctx);
        duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_NAMESERVER);
        return 1;
    }
}
void _native_vm_init_global(duk_context *ctx)
{
    duk_require_stack(ctx, 3);
    _vm_init_timer(ctx);

    // es6 shim
    duk_eval_lstring(ctx, (const char *)js_iotjs_core_es6_shim_min_js, js_iotjs_core_es6_shim_min_js_len);
    duk_push_global_object(ctx);
    duk_call(ctx, 1);
    duk_pop(ctx);

    // iotjs
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_IOTJS);
    // [..., iotjs]
    duk_push_string(ctx, VM_IOTJS_OS);
    duk_put_prop_lstring(ctx, -2, "os", 2);
    duk_push_string(ctx, VM_IOTJS_ARCH);
    duk_put_prop_lstring(ctx, -2, "arch", 4);
    duk_push_string(ctx, VM_IOTJS_VERSION);
    duk_put_prop_lstring(ctx, -2, "version", 7);
    duk_push_c_function(ctx, _native_nameserver, DUK_VARARGS);
    duk_put_prop_lstring(ctx, -2, "nameserver", 10);
    duk_push_c_lightfunc(ctx, _native_gc, 0, 0, 0);
    duk_put_prop_lstring(ctx, -2, "gc", 2);

    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    // [..., iotjs, _iotjs]
    duk_get_prop_lstring(ctx, -1, "IotError", 8);
    duk_put_prop_lstring(ctx, -3, "IotError", 8);
    duk_get_prop_lstring(ctx, -1, "getcwd", 6);
    duk_put_prop_lstring(ctx, -3, "getcwd", 6);
    duk_get_prop_lstring(ctx, -1, "exit", 4);
    duk_put_prop_lstring(ctx, -3, "exit", 4);

    duk_pop_2(ctx);
    // vm_dump_context_stdout(ctx);
    // exit(1);
}