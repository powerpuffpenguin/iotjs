#include <iotjs/core/vm.h>
#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <iotjs/core/timer.h>
#include <iotjs/core/xxd.h>

#include <duk_module_node.h>
#include <duk_console.h>

#include <errno.h>
#include <sys/stat.h>

void _native_vm_init_stash(duk_context *ctx, duk_context *main);
void _native_vm_init_global(duk_context *ctx);
void _native_vm_vm_read_js(duk_context *ctx, const char *path)
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

    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_C);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    //  [ requested_id, parent_id, native, c]
    duk_get_prop_lstring(ctx, -1, "module", 6);
    duk_get_prop_lstring(ctx, -1, "resolve", 7);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    //  [ requested_id, parent_id, native, c, resolve]
    duk_swap_top(ctx, 0);
    //  [ resolve, parent_id, native, c, requested_id]
    duk_call(ctx, 4);
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
    if (len >= 3 && !(memcmp(id + len - 3, ".js", 3)))
    {
        id = duk_get_string(ctx, 0);
        _native_vm_vm_read_js(ctx, id);
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

    // [iotjs]
    duk_push_heap_stash(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_IOTJS);
    _native_vm_init_stash(ctx, main);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_IOTJS);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // [iotjs]
    duk_push_global_object(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_IOTJS);
    _native_vm_init_global(ctx);
    duk_pop(ctx);

    duk_console_init(ctx, 0);

    duk_push_object(ctx);
    duk_push_c_function(ctx, _native_vm_resolve_module, 2);
    duk_put_prop_lstring(ctx, -2, "resolve", 7);
    duk_push_c_function(ctx, _native_vm_load_module, 3);
    duk_put_prop_lstring(ctx, -2, "load", 4);
    duk_module_node_init(ctx);
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
    _native_vm_vm_read_js(ctx, path);
    duk_swap_top(ctx, 0);
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
void _native_vm_init_c(duk_context *ctx)
{
    duk_require_stack(ctx, 5);
    // 檢測檔案或檔案夾是否存在，不存在返回 undefined
    duk_push_c_function(ctx, _native_vm_c_stat, 1);
    duk_put_prop_lstring(ctx, -2, "stat", 4);

    // 導入 path 用於解析模塊
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, "exports", 7);
    duk_eval_lstring(ctx, (const char *)core_path_js, core_path_js_len);
    duk_pop(ctx);
    duk_get_prop_lstring(ctx, -1, "exports", 7);
    duk_swap_top(ctx, -2);
    duk_del_prop_lstring(ctx, -1, "exports", 7);
    duk_pop(ctx);
    duk_put_prop_lstring(ctx, -2, "path", 4);

    // 導入 模塊解析函數
    duk_eval_lstring(ctx, (const char *)core_module_js, core_module_js_len);
    duk_put_prop_lstring(ctx, -2, "module", 6);
}
void _native_vm_init_stash(duk_context *ctx, duk_context *main)
{
    duk_require_stack(ctx, 3);
    duk_push_object(ctx);
    _native_vm_init_c(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_C);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_TIMEOUT);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_INTERVAL);
    duk_push_object(ctx);
    duk_put_prop_lstring(ctx, -2, VM_STASH_KEY_JOBS);

    _vm_init_context(ctx, main);
    _vm_init_native(ctx);

    // iotjs
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_IOTJS);
    duk_eval_lstring(ctx, (const char *)core_iotjs_js, core_iotjs_js_len);
    duk_get_prop_lstring(ctx, -1, VM_IOTJS_KEY_COMPLETER);
    duk_put_prop_lstring(ctx, -3, VM_IOTJS_KEY_COMPLETER);
    duk_get_prop_lstring(ctx, -1, VM_IOTJS_KEY_ERROR);
    duk_put_prop_lstring(ctx, -3, VM_IOTJS_KEY_ERROR);
    duk_pop_2(ctx);
}
void _native_vm_init_global(duk_context *ctx)
{
    duk_require_stack(ctx, 3);
    _vm_init_timer(ctx);

    // js
    duk_push_string(ctx, "(function(self){var exports=self;var module={exports:self};");
    duk_push_lstring(ctx, (const char *)core_es6_shim_js, core_es6_shim_js_len);
    duk_push_string(ctx, ";return module;})");
    duk_concat(ctx, 3);
    duk_eval(ctx);
    duk_push_global_object(ctx);
    duk_call(ctx, 1);
    duk_pop(ctx);

    // vm_dump_context_stdout(ctx);
}