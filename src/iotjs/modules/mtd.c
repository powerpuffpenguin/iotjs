#include <duktape.h>
#include <iotjs/core/js.h>
#include <iotjs/core/memory.h>
#include <iotjs/modules/js/mtd.h>
#include <iotjs/core/async.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <iotjs/core/finalizer.h>
#include <event2/event.h>
typedef struct
{
    int fd;
    mtd_info_t mtd_info;
    duk_uint8_t closed;
    struct event *ev;
} iotjs_mtd_fd_t;

static void iotjs_mtd_fd_free(void *ptr)
{
#ifdef VM_TRACE_FINALIZER
    puts(" --------- iotjs_mtd_fd_free");
#endif
    iotjs_mtd_fd_t *p = ptr;
    if (p->fd != -1 && !p->closed)
    {
        p->closed = 1;
        close(p->fd);
    }
    if (p->ev)
    {
        event_free(p->ev);
    }
}
static void iotjs_mtd_fd_handler(evutil_socket_t fd, short events, void *args)
{
    puts("iotjs_mtd_fd_handler");
}
static duk_ret_t native_open(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    duk_require_callable(ctx, 1);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

    // cb
    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(iotjs_mtd_fd_t));
    iotjs_mtd_fd_t *p = finalizer->p;
    finalizer->free = iotjs_mtd_fd_free;
    p->fd = open(path, write ? O_RDWR : O_RDONLY);
    if (p->fd == -1)
    {
        duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    }
    if (ioctl(p->fd, MEMGETINFO, &p->mtd_info) == -1)
    {
        vm_finalizer_free(ctx, -1, iotjs_mtd_fd_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    }
    vm_context_t *vm = vm_get_context(ctx);
    p->ev = event_new(vm->eb, -1, EV_PERSIST, iotjs_mtd_fd_handler, p);
    if (!p->ev)
    {
        duk_error(ctx, DUK_ERR_ERROR, "event_new error");
    }
    if (event_add(p->ev, NULL))
    {
        duk_error(ctx, DUK_ERR_ERROR, "event_add error");
    }

    // cb, finalizer
    vm_snapshot_copy(ctx, VM_SNAPSHOT_MTD, p, 2);
    return 1;
}
static duk_ret_t native_close(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    if (p->fd != -1 && !p->closed)
    {
        p->closed = 1;
        close(p->fd);
    }
    return 0;
}
static duk_ret_t native_free(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    vm_remove_snapshot(ctx, VM_SNAPSHOT_MTD, finalizer->p);
    vm_finalizer_free(ctx, 0, iotjs_mtd_fd_free);
    return 0;
}
static duk_ret_t native_info(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    duk_pop(ctx);
    duk_push_object(ctx);
    duk_push_number(ctx, p->mtd_info.type);
    duk_put_prop_lstring(ctx, -2, "type", 4);
    duk_push_number(ctx, p->mtd_info.flags);
    duk_put_prop_lstring(ctx, -2, "flags", 5);
    duk_push_number(ctx, p->mtd_info.size);
    duk_put_prop_lstring(ctx, -2, "size", 4);
    duk_push_number(ctx, p->mtd_info.erasesize);
    duk_put_prop_lstring(ctx, -2, "erasesize", 9);
    duk_push_number(ctx, p->mtd_info.writesize);
    duk_put_prop_lstring(ctx, -2, "writesize", 9);
    duk_push_number(ctx, p->mtd_info.oobsize);
    duk_put_prop_lstring(ctx, -2, "oobsize", 7);
    return 1;
}
static duk_ret_t native_seek_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    off_t offset = duk_require_number(ctx, 1);
    int whence = duk_require_number(ctx, 2);
    switch (whence)
    {
    case 0:
        whence = SEEK_SET;
        break;
    case 1:
        whence = SEEK_CUR;
        break;
    case 2:
        whence = SEEK_END;
        break;
    default:
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "seek unknow whence %d", whence);
        break;
    }
    offset = lseek(p->fd, offset, whence);
    if (offset == -1)
    {
        duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    }
    duk_push_number(ctx, offset);
    return 1;
}
static duk_ret_t native_read_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    duk_size_t sz;
    void *buf = duk_require_buffer_data(ctx, 1, &sz);
    if (!sz)
    {
        duk_pop_2(ctx);
        duk_push_number(ctx, 0);
        return 1;
    }
    ssize_t ok = read(p->fd, buf, sz);
    if (ok == -1)
    {
        duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    }
    duk_pop_2(ctx);
    duk_push_number(ctx, ok);
    return 1;
}
static duk_ret_t native_write_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    duk_size_t sz;
    void *buf = duk_require_buffer_data(ctx, 1, &sz);
    if (!sz)
    {
        duk_pop_2(ctx);
        duk_push_number(ctx, 0);
        return 1;
    }
    ssize_t ok = write(p->fd, buf, sz);
    if (ok == -1)
    {
        duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    }
    duk_pop_2(ctx);
    duk_push_number(ctx, ok);
    return 1;
}

static duk_ret_t native_seek(duk_context *ctx)
{
    // finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    // iotjs_mtd_fd_t *p = finalizer->p;
    // off_t offset = duk_require_number(ctx, 1);
    // int whence = duk_require_number(ctx, 2);
    // switch (whence)
    // {
    // case 0:
    //     whence = SEEK_SET;
    //     break;
    // case 1:
    //     whence = SEEK_CUR;
    //     break;
    // case 2:
    //     whence = SEEK_END;
    //     break;
    // default:
    //     duk_error(ctx, DUK_ERR_TYPE_ERROR, "seek unknow whence %d", whence);
    //     break;
    // }
    // offset = lseek(p->fd, offset, whence);
    // if (offset == -1)
    // {
    //     duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    // }
    // duk_push_number(ctx, offset);
    return 1;
}
static void do_read_async(vm_context_t *vm, void *args)
{
}
static duk_ret_t native_read(duk_context *ctx)
{
    // finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    // iotjs_mtd_fd_t *p = finalizer->p;
    // duk_size_t sz;
    // void *buf = duk_require_buffer_data(ctx, 1, &sz);
    // if (!sz)
    // {
    //     duk_pop_2(ctx);
    //     duk_push_number(ctx, 0);
    //     return 1;
    // }
    // p->sz = sz;
    // p->buf = buf;
    // vm_async(ctx, do_read_async, p);
    return 0;
}
static duk_ret_t native_write(duk_context *ctx)
{
    // finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    // iotjs_mtd_fd_t *p = finalizer->p;
    // duk_size_t sz;
    // void *buf = duk_require_buffer_data(ctx, 1, &sz);
    // ssize_t ok = sz == 0 ? 0 : write(p->fd, buf, sz);
    // if (ok == -1)
    // {
    //     duk_error(ctx, DUK_ERR_ERROR, strerror(errno));
    // }
    // duk_pop_2(ctx);
    // duk_push_number(ctx, ok);
    return 1;
}

duk_ret_t native_iotjs_mtd_init(duk_context *ctx)
{
    duk_swap(ctx, 0, 1);
    duk_pop_2(ctx);

    duk_eval_lstring(ctx, (const char *)js_iotjs_modules_js_mtd_min_js, js_iotjs_modules_js_mtd_min_js_len);
    duk_swap_top(ctx, -2);
    duk_push_heap_stash(ctx);
    duk_get_prop_lstring(ctx, -1, VM_STASH_KEY_PRIVATE);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_push_object(ctx);
    {
        duk_push_object(ctx);
        duk_push_number(ctx, 0);
        duk_put_prop_lstring(ctx, -2, "set", 3);
        duk_push_number(ctx, 1);
        duk_put_prop_lstring(ctx, -2, "cur", 3);
        duk_push_number(ctx, 2);
        duk_put_prop_lstring(ctx, -2, "end", 3);
        duk_put_prop_lstring(ctx, -2, "Seek", 4);

        duk_push_c_lightfunc(ctx, native_open, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "open", 4);
        duk_push_c_lightfunc(ctx, native_close, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "close", 5);
        duk_push_c_lightfunc(ctx, native_free, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "free", 4);

        duk_push_c_lightfunc(ctx, native_info, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "info", 4);

        duk_push_c_lightfunc(ctx, native_seek_sync, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "seekSync", 8);
        duk_push_c_lightfunc(ctx, native_read_sync, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "readSync", 8);
        duk_push_c_lightfunc(ctx, native_write_sync, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "writeSync", 9);

        duk_push_c_lightfunc(ctx, native_seek, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "seek", 4);
        duk_push_c_lightfunc(ctx, native_read, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "read", 4);
        duk_push_c_lightfunc(ctx, native_write, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "write", 5);
    }
    duk_call(ctx, 3);
    return 0;
}