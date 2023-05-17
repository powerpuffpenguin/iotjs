#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <iotjs/core/number.h>
#include <sys/stat.h>
#include <errno.h>
void _async_fs_stat_work(vm_async_job_t *job, void *in)
{
    const char *path = in;
    struct stat *out = job->out;
    if (stat(path, out))
    {
        job->err = errno;
    }

    vm_complete_async_job(job);
}
duk_ret_t _async_fs_stat_complete(duk_context *ctx)
{
    vm_async_job_t *job = (vm_async_job_t *)vm_get_async_job(ctx);
    switch (job->err)
    {
    case 0:
        duk_push_object(ctx);
        {
            duk_get_prop_lstring(ctx, -2, "args", 4);
            vm_path(ctx);
            duk_swap_top(ctx, -2);
            duk_get_prop_lstring(ctx, -2, "base", 4);
            duk_swap_top(ctx, -3);
            duk_pop(ctx);
            duk_call(ctx, 1);
            duk_put_prop_lstring(ctx, -2, "name", 4);

            struct stat *info = (struct stat *)job->out;
            vm_push_size(ctx, info->st_size);
            duk_put_prop_lstring(ctx, -2, "size", 4);

            vm_require_date(ctx);
            uint64_t mtime = info->st_mtime;
            vm_push_uint64(ctx, mtime * 1000);
            duk_new(ctx, 1);
            duk_put_prop_lstring(ctx, -2, "modTime", 7);

            duk_push_boolean(ctx, S_ISDIR(info->st_mode));
            duk_put_prop_lstring(ctx, -2, "isDir", 5);
        }
        vm_resolve_async_job(ctx, -2);
        break;
    case ENOENT:
        duk_push_undefined(ctx);
        vm_resolve_async_job(ctx, -2);
        break;
    default:
        duk_push_error_object(ctx, DUK_ERR_ERROR, strerror(job->err));
        vm_reject_async_job(ctx, -2);
        break;
    }
    return 0;
}
duk_ret_t _native_iotjs_fs_stat(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    vm_async_job_t *job = vm_new_async_job(ctx,
                                           _async_fs_stat_work,
                                           0, sizeof(struct stat));
    job->in = (void *)path;
    job->complete = _async_fs_stat_complete;
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "args", 4);
    vm_execute_async_job(ctx, job);
    return 1;
}
duk_ret_t native_iotjs_fs_init(duk_context *ctx)
{
    duk_push_c_function(ctx, _native_iotjs_fs_stat, 1);
    duk_put_prop_string(ctx, 1, "stat");
    return 0;
}
