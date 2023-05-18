#include <iotjs/core/js.h>
#include <iotjs/core/module.h>
#include <iotjs/core/number.h>
#include <sys/stat.h>
#include <errno.h>
// ... name => return
void _native_iotjs_fs_stat_push(duk_context *ctx, struct stat *info)
{
    vm_path(ctx);
    duk_swap_top(ctx, -2);
    duk_get_prop_lstring(ctx, -2, "base", 4);
    duk_swap_top(ctx, -3);
    duk_pop(ctx);
    duk_call(ctx, 1);

    duk_push_object(ctx);
    duk_swap_top(ctx, -2);
    duk_put_prop_lstring(ctx, -2, "name", 4);

    vm_push_size(ctx, info->st_size);
    duk_put_prop_lstring(ctx, -2, "size", 4);

    vm_require_date(ctx);
    uint64_t mtime = info->st_mtime;
    vm_push_uint64(ctx, mtime * 1000);
    duk_new(ctx, 1);
    duk_put_prop_lstring(ctx, -2, "mtime", 5);
    int isDir = 0;

    duk_uint32_t mode = info->st_mode & 0777;
    switch (info->st_mode & IOTJS_SYSCALL_S_IFMT)
    {
    case IOTJS_SYSCALL_S_IFBLK:
        mode |= IOTJS_FILEMODE_DEVICE;
        break;
    case IOTJS_SYSCALL_S_IFCHR:
        mode |= IOTJS_FILEMODE_DEVICE | IOTJS_FILEMODE_CHAR_DEVICE;
        break;
    case IOTJS_SYSCALL_S_IFDIR:
        mode |= IOTJS_FILEMODE_DIR;
        isDir = 1;
        break;
    case IOTJS_SYSCALL_S_IFIFO:
        mode |= IOTJS_FILEMODE_PIPE;
        break;
    case IOTJS_SYSCALL_S_IFLNK:
        mode |= IOTJS_FILEMODE_LINK;
        break;
    case IOTJS_SYSCALL_S_IFREG:
        // nothing to do
        break;
    case IOTJS_SYSCALL_S_IFSOCK:
        mode |= IOTJS_FILEMODE_SOCKET;
        break;
    }
    if (info->st_mode & IOTJS_SYSCALL_S_ISGID)
    {
        mode |= IOTJS_FILEMODE_SETGID;
    }
    if (info->st_mode & IOTJS_SYSCALL_S_ISUID)
    {
        mode |= IOTJS_FILEMODE_SETUID;
    }
    if (info->st_mode & IOTJS_SYSCALL_S_ISVTX)
    {
        mode |= IOTJS_FILEMODE_STICKY;
    }
    duk_push_boolean(ctx, isDir);
    duk_put_prop_lstring(ctx, -2, "isDir", 5);
    duk_push_number(ctx, mode);
    duk_put_prop_lstring(ctx, -2, "mode", 4);
}
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
        duk_get_prop_lstring(ctx, -1, "args", 4);
        _native_iotjs_fs_stat_push(ctx, job->out);
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
duk_ret_t _native_iotjs_fs_stat_sync(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    struct stat info;
    if (stat(path, &info))
    {
        int err = errno;
        duk_pop(ctx);
        if (err != ENOENT)
        {
            duk_push_error_object(ctx, DUK_ERR_ERROR, strerror(err));
            duk_throw(ctx);
        }
        duk_push_undefined(ctx);
        return 1;
    }
    _native_iotjs_fs_stat_push(ctx, &info);
    return 1;
}
duk_ret_t native_iotjs_fs_init(duk_context *ctx)
{
    duk_push_c_function(ctx, _native_iotjs_fs_stat, 1);
    duk_put_prop_lstring(ctx, 1, "stat", 4);
    duk_push_c_function(ctx, _native_iotjs_fs_stat_sync, 1);
    duk_put_prop_lstring(ctx, 1, "statSync", 8);

    duk_push_object(ctx);
    duk_push_number(ctx, IOTJS_FILEMODE_DIR);
    duk_put_prop_lstring(ctx, -2, "dir", 3);
    duk_push_number(ctx, IOTJS_FILEMODE_APPEND);
    duk_put_prop_lstring(ctx, -2, "append", 6);
    duk_push_number(ctx, IOTJS_FILEMODE_EXCLUSIVE);
    duk_put_prop_lstring(ctx, -2, "exclusive", 9);
    duk_push_number(ctx, IOTJS_FILEMODE_TEMPORARY);
    duk_put_prop_lstring(ctx, -2, "temporary", 9);
    duk_push_number(ctx, IOTJS_FILEMODE_LINK);
    duk_put_prop_lstring(ctx, -2, "link", 4);
    duk_push_number(ctx, IOTJS_FILEMODE_DEVICE);
    duk_put_prop_lstring(ctx, -2, "device", 6);
    duk_push_number(ctx, IOTJS_FILEMODE_PIPE);
    duk_put_prop_lstring(ctx, -2, "pipe", 4);
    duk_push_number(ctx, IOTJS_FILEMODE_SOCKET);
    duk_put_prop_lstring(ctx, -2, "socket", 6);
    duk_push_number(ctx, IOTJS_FILEMODE_SETUID);
    duk_put_prop_lstring(ctx, -2, "setuid", 6);
    duk_push_number(ctx, IOTJS_FILEMODE_SETGID);
    duk_put_prop_lstring(ctx, -2, "setgid", 6);
    duk_push_number(ctx, IOTJS_FILEMODE_CHAR_DEVICE);
    duk_put_prop_lstring(ctx, -2, "charDevice", 10);
    duk_push_number(ctx, IOTJS_FILEMODE_IR_REG);
    duk_put_prop_lstring(ctx, -2, "irregular", 9);
    duk_push_number(ctx, IOTJS_FILEMODE_STICKY);
    duk_put_prop_lstring(ctx, -2, "sticky", 6);

    duk_push_number(ctx, IOTJS_FILEMODE_TYPE);
    duk_put_prop_lstring(ctx, -2, "type", 4);
    duk_push_number(ctx, IOTJS_FILEMODE_PERM);
    duk_put_prop_lstring(ctx, -2, "perm", 4);

    duk_put_prop_lstring(ctx, 1, "FileMode", 8);
    return 0;
}
