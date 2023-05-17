#include <iotjs/core/vm.h>
#include <iotjs/core/js.h>

typedef struct
{

} fs_stat_t;
void _async_fs_stat(void *arg)
{
    // vm_async_t *p = (vm_async_t *)arg;
    // event_t *ev = IOTJS_ASYNC_EVENT(p);
    // event_active(ev, 0, 0);
}
duk_ret_t _native_iotjs_fs_stat(duk_context *ctx)
{
    // const char *path = duk_require_string(ctx, 0);
    // size_t sz_in = sizeof(char *);
    // vm_async_t *p = (vm_async_t *)vm_new_async(ctx,
    //                                            sz_in,
    //                                            sizeof(fs_stat_t));
    // duk_dup(ctx, 0);
    // duk_put_prop_string(ctx, -2, "path");

    // const char **in = (const char **)IOTJS_ASYNC_IN(p);
    // *in = path;

    // event_t *ev = IOTJS_ASYNC_EVENT(p);
    // struct timeval tv = {
    //     .tv_sec = 0,
    //     .tv_usec = 0,
    // };
    // if (event_add(ev, &tv))
    // {
    //     duk_pop_3(ctx);
    //     duk_push_error_object(ctx, DUK_ERR_ERROR, "event_add error");
    //     duk_throw(ctx);
    // }
    // if (thpool_add_work(p->threads, _async_fs_stat, p))
    // {
    //     duk_pop_3(ctx);
    //     duk_push_error_object(ctx, DUK_ERR_ERROR, "event_add error");
    //     duk_throw(ctx);
    // }
    // duk_put_prop(ctx, -3);
    return 1;
}
duk_ret_t native_iotjs_fs_init(duk_context *ctx)
{
    duk_push_c_function(ctx, _native_iotjs_fs_stat, 1);
    duk_put_prop_string(ctx, 1, "stat");
    return 0;
}
