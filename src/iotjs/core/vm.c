#include <iotjs/core/vm.h>
#include <iotjs/core/strings.h>
#include <iotjs/core/path.h>

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
void vm_dump_context_stdout(duk_context *ctx)
{
    duk_push_context_dump(ctx);
    fprintf(stdout, "%s\n", duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
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
    duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "resolve");
    duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
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
duk_ret_t vm_main(duk_context *ctx, const char *path)
{
    duk_push_heap_stash(ctx);
    duk_push_pointer(ctx, ctx);
    duk_put_prop_string(ctx, -2, "ctx");
    duk_pop(ctx);

    duk_push_c_function(ctx, _vm_iotjs_main, 1);
    duk_push_string(ctx, path);
    return duk_pcall(ctx, 1);
}
duk_context *vm_context(duk_context *ctx)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "ctx");
    if (!duk_is_pointer(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.ctx invalid");
        duk_throw(ctx);
    }
    duk_context *c = duk_get_pointer(ctx, -1);
    duk_pop_2(ctx);
    return c;
}
event_base_t *vm_event_base(duk_context *ctx)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "events");
    if (!duk_is_object(ctx, -1))
    {
        duk_pop_2(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events invalid");
        duk_throw(ctx);
    }
    if (!duk_get_prop_string(ctx, -1, "eb"))
    {
        duk_pop_3(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events.eb not exists");
        duk_throw(ctx);
    }
    if (!duk_is_pointer(ctx, -1))
    {
        duk_pop_3(ctx);
        duk_push_error_object(ctx, DUK_ERR_EVAL_ERROR, "stash.events.eb invalid");
        duk_throw(ctx);
    }
    event_base_t *eb = (event_base_t *)duk_get_pointer(ctx, -1);
    duk_pop_3(ctx);
    return eb;
}
duk_ret_t cb_resolve_module(duk_context *ctx)
{
    /*
     *  Entry stack: [ requested_id parent_id ]
     */

#ifdef VM_DEBUG_MODULE_LOAD
    puts("*** cb_resolve_module");
    vm_dump_context_stdout(ctx);
#endif
    size_t requested_len;
    const char *requested_id = duk_get_lstring(ctx, 0, &requested_len);
    if (!requested_len)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "require expects a valid module id");
        duk_throw(ctx);
    }

    string_t resolved_id;
    const char *type;           // 模塊類型
    if (requested_id[0] == '.') // 加載相對位置的模塊
    {
        type = "0";
        size_t parent_len;
        const char *parent_id = duk_get_lstring(ctx, 1, &parent_len); /* calling module */

        if (parent_len)
        {
            int i = parent_len - 1;
            while (i >= 0 && parent_id[i] != '/')
            {
                i--;
            }
            if (i > -1)
            {
                parent_len = i + 1;
            }
            else
            {
                parent_len = 0;
            }
        }

        /* Arrive at the canonical module ID somehow. */
        if (parent_len)
        {
            size_t n = parent_len + requested_len;
            resolved_id = strings_make(n);
            if (IOTJS_REFERENCE_INVALID(resolved_id))
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "require strings_make() error");
                duk_throw(ctx);
            }
            char *p = IOTJS_REFERENCE_PTR(resolved_id);
            memcpy(p, parent_id, parent_len);
            memcpy(p + parent_len, requested_id, requested_len);

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
    }
    else // 加載系統模塊
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

        duk_push_heap_stash(ctx);
        duk_get_prop_string(ctx, -1, "native");
        duk_get_prop_lstring(ctx, -1, IOTJS_REFERENCE_PTR(resolved_id), resolved_id.len);
        if (duk_is_c_function(ctx, -1))
        {
            type = "2";
        }
        else
        {
            type = "1";
            string_t s = strings_make_cap(0, resolved_id.len + 13);
            if (IOTJS_REFERENCE_INVALID(s))
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "require strings_make_cap() error");
                duk_throw(ctx);
            }
            s = strings_append_str(&s, "node_modules/", 13, TRUE);
            resolved_id = strings_append(&s, &resolved_id, TRUE, TRUE);
            if (IOTJS_REFERENCE_INVALID(resolved_id))
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "require strings_append() error");
                duk_throw(ctx);
            }
            resolved_id = path_clean(&resolved_id, TRUE);
            if (IOTJS_REFERENCE_INVALID(resolved_id))
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "require path_clean(resolved_id) error");
                duk_throw(ctx);
            }
        }
        duk_pop_3(ctx);
    }
    duk_push_lstring(ctx, type, 1);
    duk_push_lstring(ctx, IOTJS_REFERENCE_PTR(resolved_id), resolved_id.len);
    strings_decrement(&resolved_id);
    duk_concat(ctx, 2);
    return 1; /*nrets*/
}
duk_ret_t cb_load_module(duk_context *ctx)
{

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

    switch (id[0])
    {
    case '0': // load js
    {
        duk_push_lstring(ctx, id + 1, len - 1);
        duk_push_lstring(ctx, ".js", 3);
        duk_concat(ctx, 2);
        duk_dup_top(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, 2, "filename");
        duk_put_prop_string(ctx, 2, "id");
        id = duk_get_string(ctx, -1);
        vm_read_js(ctx, id);
    }
    break;
    case '1': // load node_modules js
    {
        const char *filepath = duk_get_string(ctx, 0);
        struct stat fsstat;
        int err = 0;
        if (stat(filepath + 1, &fsstat))
        {
            err = errno;
            if (err != ENOENT)
            {
                duk_push_error_object(ctx, DUK_ERR_ERROR, "stat error(%d): %s", err, strerror(err));
                duk_throw(ctx);
            }
        }
        if (err) // file
        {
            duk_push_lstring(ctx, id + 1, len - 1);
            duk_push_lstring(ctx, ".js", 3);
            duk_concat(ctx, 2);
            duk_dup_top(ctx);
            duk_dup_top(ctx);
            duk_put_prop_string(ctx, 2, "filename");
            duk_put_prop_string(ctx, 2, "id");
            id = duk_get_string(ctx, -1);
        }
        else if (S_ISDIR(fsstat.st_mode))
        {
            duk_push_lstring(ctx, id + 1, len - 1);
            duk_push_string(ctx, "/index.js");
            duk_concat(ctx, 2);
            duk_dup_top(ctx);
            duk_dup_top(ctx);
            duk_put_prop_string(ctx, 2, "filename");
            duk_put_prop_string(ctx, 2, "id");
            id = duk_get_string(ctx, -1);
        }
        else
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, "module not exists: %s", filepath + 1);
            duk_throw(ctx);
        }
        id = duk_get_string(ctx, -1);
        vm_read_js(ctx, id);
        duk_buffer_to_string(ctx, -1);
    }
    break;
    case '2': // load c modules
        duk_push_lstring(ctx, id + 1, len - 1);
        duk_dup_top(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, 2, "filename");
        duk_put_prop_string(ctx, 2, "id");
        duk_swap_top(ctx, 0);
        duk_pop(ctx);
        id = duk_get_lstring(ctx, 0, &len);
        duk_push_heap_stash(ctx);
        duk_get_prop_string(ctx, -1, "native");
        duk_dup(ctx, 0);
        duk_get_prop(ctx, -2);
        if (duk_is_c_function(ctx, -1))
        {
            duk_c_function f = duk_get_c_function(ctx, -1);
            duk_pop_3(ctx);
            return f(ctx);
        }
        duk_push_error_object(ctx, DUK_ERR_ERROR, "native module not exists");
        duk_throw(ctx);
    default:
        duk_push_error_object(ctx, DUK_ERR_ERROR, "cb_load_module unknow module type");
        duk_throw(ctx);
    }
    return 1; /*nrets*/
}
typedef struct
{
    struct event *ev;
    duk_context *ctx;
    uint8_t interval;
} _vm_timer_t;
void _vm_timer_handler(evutil_socket_t fs, short events, void *arg)
{
    _vm_timer_t *timer = (_vm_timer_t *)arg;
    duk_context *ctx = timer->ctx;
    // timer
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_get_prop(ctx, -2);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    if (!duk_is_object(ctx, -1))
    {
        duk_pop(ctx);
        return;
    }
    // cb
    duk_get_prop_string(ctx, -1, "cb");
    if (!duk_is_function(ctx, -1))
    {
        duk_pop_2(ctx);
        return;
    }
    if (duk_pcall(ctx, 0))
    {
        puts(duk_safe_to_string(ctx, -1));
        abort();
    }
    duk_pop_2(ctx);
    if (timer->interval)
    {
        return;
    }
    // remove
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_del_prop(ctx, -2);
}
duk_ret_t _vm_timer_finalizer(duk_context *ctx)
{
    duk_get_prop_string(ctx, 0, "timer");
    _vm_timer_t *timer = (_vm_timer_t *)duk_get_pointer(ctx, -1);
    if (timer->ev)
    {
        event_del(timer->ev);
    }
    free(timer);
    return 0;
}
_vm_timer_t *_vm_new_timer(duk_context *ctx, duk_int_t ms)
{
    // timer
    struct event_base *eb = vm_event_base(ctx);
    duk_context *cm = vm_context(ctx);
    _vm_timer_t *timer = (_vm_timer_t *)malloc(sizeof(_vm_timer_t));
    if (!timer)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "malloc timer error");
        duk_throw(ctx);
    }
    memset(timer, 0, sizeof(_vm_timer_t));
    timer->ctx = cm;
    duk_push_object(ctx);
    duk_push_pointer(ctx, timer);
    duk_put_prop_string(ctx, -2, "timer");
    duk_push_c_function(ctx, _vm_timer_finalizer, 1);
    duk_set_finalizer(ctx, -2);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "cb");

    // event
    timer->ev = event_new(eb, -1, EV_TIMEOUT, _vm_timer_handler, timer);
    if (!timer->ev)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_new timer error");
        duk_throw(ctx);
    }
    struct timeval tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    if (event_add(timer->ev, &tv))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "event_add timer error");
        duk_throw(ctx);
    }
    return timer;
}
void _vm_set_timer(duk_context *ctx, _vm_timer_t *timer)
{
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "timer");
    if (!duk_is_object(ctx, -1))
    {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, -3, "timer");
    }
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_pointer(ctx, timer);
    duk_safe_to_string(ctx, -1);
    duk_dup(ctx, 0);
    duk_put_prop(ctx, -3);
    duk_pop(ctx);
}
duk_ret_t _vm_iotjs_nativa_setTimeout(duk_context *ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs < 1 || !duk_is_function(ctx, 0))
    {
        return 0;
    }
    duk_int_t ms = 0;
    if (duk_is_number(ctx, 1))
    {
        ms = duk_get_int(ctx, -1);
        if (ms < 0)
        {
            ms = 0;
        }
    }
    if (nargs > 1)
    {
        duk_pop_n(ctx, nargs - 1);
    }
    _vm_timer_t *timer = _vm_new_timer(ctx, ms);
    duk_swap_top(ctx, 0);
    duk_pop(ctx);
    _vm_set_timer(ctx, timer);

    return 1;
}
void _vm_iotjs_init_compatible(duk_context *ctx)
{
    duk_push_c_function(ctx, _vm_iotjs_nativa_setTimeout, 2);
    duk_put_prop_string(ctx, -2, "setTimeout");
}