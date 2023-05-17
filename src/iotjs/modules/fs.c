#include <iotjs/core/vm.h>
#include <iotjs/core/js.h>
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
    if (job->err)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, strerror(job->err));
        vm_reject_async_job(ctx, -2);
        return 0;
    }
    vm_dump_context_stdout(ctx);
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
