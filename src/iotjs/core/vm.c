#include <iotjs/core/vm.h>
#include <iotjs/core/js_timer.h>
#include <iotjs/core/js_process.h>
#include <iotjs/core/xxd.h>
#include <iotjs/core/js_threads.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "duk_module_node.h"
#include "duk_console.h"

duk_ret_t cb_resolve_module(duk_context *ctx);
duk_ret_t cb_load_module(duk_context *ctx);
duk_ret_t _vm_iotjs_main(duk_context *ctx);
void _vm_iotjs_init_compatible(duk_context *ctx);
void _vm_iotjs_init_stash(duk_context *ctx);

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
void vm_register(const char *name, duk_c_function init)
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

void vm_read_js(duk_context *ctx, const char *path)
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
duk_ret_t _vm_iotjs_events_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, -1, "eb");
    event_base_t *eb = (event_base_t *)duk_get_pointer(ctx, -1);
    event_base_free(eb);
    return 0;
}
duk_ret_t _vm_iotjs_main(duk_context *ctx)
{

    duk_push_heap_stash(ctx);
    _vm_iotjs_init_stash(ctx);
    // 創建 stash.events 存儲 事件驅動
    {
        event_base_t *eb = event_base_new();
        if (!eb)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "event_base_new error");
            duk_throw(ctx);
        }
        duk_push_object(ctx);
        duk_push_pointer(ctx, eb);
        duk_put_prop_string(ctx, -2, "eb");
        duk_push_c_function(ctx, _vm_iotjs_events_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "events");
    }
    // 創建 stash.native 存儲 c 模塊
    {
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
        duk_put_prop_string(ctx, -2, "native");
    }
    // vm_dump_context_stdout(ctx);
    duk_pop(ctx);
    // 初始化 commonjs 模塊引擎
    duk_push_object(ctx);
    duk_push_c_function(ctx, cb_resolve_module, 2);
    duk_put_prop_string(ctx, -2, "resolve");
    duk_push_c_function(ctx, cb_load_module, 3);
    duk_put_prop_string(ctx, -2, "load");
    duk_module_node_init(ctx);
    // 初始化 console
    duk_console_init(ctx, 0);

    duk_push_global_object(ctx);
    _vm_iotjs_init_compatible(ctx);
    duk_pop(ctx);

    const char *path = duk_get_string(ctx, 0);
    vm_read_js(ctx, path);
    duk_swap_top(ctx, 0);
    // vm_dump_context_stdout(ctx);
    if (duk_module_node_peval_main(ctx, path))
    {
        duk_throw(ctx);
    }
    return 1;
}
duk_ret_t _iotjs_fs_stat(duk_context *ctx)
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
duk_ret_t vm_main(duk_context *ctx, const char *path, int argc, char *argv[])
{
    // js
    duk_push_lstring(ctx, (const char *)core_js_js, core_js_js_len);
    if (duk_peval(ctx))
    {
        return DUK_EXEC_ERROR;
    }
    duk_pop(ctx);

    // stash
    duk_push_heap_stash(ctx);
    {
        // ctx
        duk_push_pointer(ctx, ctx);
        duk_put_prop_string(ctx, -2, "ctx");

        // argv
        duk_push_array(ctx);
        for (int i = 0; i < argc; i++)
        {
            duk_push_string(ctx, argv[i]);
            duk_put_prop_index(ctx, -2, i);
        }
        duk_put_prop_string(ctx, -2, "argv");

        // path
        duk_push_string(ctx, "(function(){var exports={};");
        duk_push_lstring(ctx, (char *)core_path_js, core_path_js_len);
        duk_push_string(ctx, "return exports;})()");
        duk_concat(ctx, 3);
        if (duk_peval(ctx))
        {
            return DUK_EXEC_ERROR;
        }
        duk_put_prop_string(ctx, -2, "path");

        // module
        duk_push_lstring(ctx, (char *)core_js_module_js, core_js_module_js_len);
        if (duk_peval(ctx))
        {
            return DUK_EXEC_ERROR;
        }
        duk_put_prop_string(ctx, -2, "module");

        // c func
        duk_push_object(ctx);
        duk_push_c_function(ctx, _iotjs_fs_stat, 1);
        duk_put_prop_string(ctx, -2, "stat");
        duk_put_prop_string(ctx, -2, "c_func");
    }
    duk_pop(ctx);

    duk_push_c_function(ctx, _vm_iotjs_main, 1);
    duk_push_string(ctx, path);
    return duk_pcall(ctx, 1);
}

duk_ret_t cb_resolve_module(duk_context *ctx)
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
    duk_get_prop_string(ctx, -1, "module");
    duk_get_prop_string(ctx, -1, "resolve_module");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap(ctx, 1, 2);
    duk_swap(ctx, 0, 1);
    duk_swap_top(ctx, 0);

    // [resolve_module, requested_id, parent_id, stash]
    // vm_dump_context_stdout(ctx);
    duk_get_prop_string(ctx, -1, "native");
    duk_get_prop_string(ctx, -2, "c_func");
    duk_get_prop_string(ctx, -3, "path");
    duk_swap_top(ctx, -4);
    duk_pop(ctx);
    duk_call(ctx, 5);

    return 1; /*nrets*/
}
duk_ret_t cb_load_module(duk_context *ctx)
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
        vm_read_js(ctx, id);
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

void _vm_iotjs_init_compatible(duk_context *ctx)
{
    _vm_init_process(ctx);
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
}
void _vm_iotjs_init_stash(duk_context *ctx)
{
    vm_init_context(ctx);

    // js
    duk_eval_lstring(ctx, (const char *)core_completer_js, core_completer_js_len);
    duk_put_prop_string(ctx, -2, "completer");
    _vm_init_threads(ctx);
}
void _vm_async_callbackup(evutil_socket_t fd, short events, void *arg)
{
    vm_async_t *p = (vm_async_t *)arg;
    p->complete(p);
}
vm_async_t *vm_new_async(duk_context *ctx, size_t in, size_t out)
{
    size_t sz_event = event_get_struct_event_size();
    size_t sz = sizeof(vm_async_t) + sz_event + in + out;
    vm_async_t *p = (vm_async_t *)malloc(sz);
    if (!p)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "vm_new_async malloc error");
        duk_throw(ctx);
    }
    char *ptr = ((char *)p) + sizeof(vm_async_t);
    p->ev = (event_t *)ptr;
    ptr += sz_event;
    if (in)
    {
        p->in = ptr;
        ptr += in;
    }
    else
    {
        p->in = NULL;
    }
    p->out = out ? ptr : NULL;

    event_assign(p->ev, p->eb, -1, 0, _vm_async_callbackup, p);
    return p;
}