#include <duktape.h>
#include <iotjs/core/js.h>
#include <iotjs/core/memory.h>
#include <iotjs/modules/js/mtd.h>
#include <iotjs/core/async.h>
#include <iotjs/core/binary.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <iotjs/core/finalizer.h>
#include <event2/event.h>

#include <iotjs/modules/mtd_db.h>

IOTJS_SPIFFS_DEFINE(0)
IOTJS_SPIFFS_DEFINE(1)
IOTJS_SPIFFS_DEFINE(2)

typedef struct
{
    int fd;
    mtd_info_t mtd_info;
    duk_uint8_t closed;
    struct event *ev;
    vm_job_t *job;
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
typedef struct
{
    iotjs_mtd_fd_t *mtd;
    duk_size_t sz;
    void *buf;
} async_in_t;
typedef struct
{
    ssize_t ret;
    int err;
} async_out_t;
typedef struct
{
    iotjs_mtd_fd_t *mtd;
    off_t offset;
    int whence;
} async_seek_t;
typedef struct
{
    iotjs_mtd_fd_t *mtd;
    unsigned int start;
    unsigned int length;
} async_erase_t;

static void iotjs_mtd_fd_handler(evutil_socket_t fd, short events, void *args)
{
    if (events)
    {
        return;
    }

    iotjs_mtd_fd_t *p = args;
    vm_job_t *job = p->job;
    if (!job)
    {
        return;
    }
    p->job = NULL;
    duk_context *ctx = job->vm->ctx;

    async_out_t *out = job->out;
    vm_restore(ctx, VM_SNAPSHOT_MTD, p, 0);
    duk_pop(ctx);
    duk_push_number(ctx, events);
    if (out->ret == -1)
    {
        duk_push_undefined(ctx);
        duk_push_error_object(ctx, DUK_ERR_ERROR, strerror(out->err));
        duk_call(ctx, 3);
    }
    else
    {
        duk_push_number(ctx, out->ret);
        duk_call(ctx, 2);
    }
    duk_pop(ctx);

    vm_free(job);
}
static duk_ret_t native_open(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    duk_require_callable(ctx, 1);
    duk_swap_top(ctx, 0);
    duk_pop(ctx);

    // cb
    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(iotjs_mtd_fd_t));
    iotjs_mtd_fd_t *p = finalizer->p;
    finalizer->free = iotjs_mtd_fd_free;
    p->fd = open(path, O_RDWR);
    if (p->fd == -1)
    {
        int err = errno;
        vm_finalizer_free(ctx, -1, iotjs_mtd_fd_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    if (ioctl(p->fd, MEMGETINFO, &p->mtd_info) == -1)
    {
        int err = errno;
        vm_finalizer_free(ctx, -1, iotjs_mtd_fd_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    vm_context_t *vm = vm_get_context(ctx);
    p->ev = event_new(vm->eb, -1, EV_PERSIST | EV_TIMEOUT, iotjs_mtd_fd_handler, p);
    if (!p->ev)
    {
        vm_finalizer_free(ctx, -1, iotjs_mtd_fd_free);
        duk_error(ctx, DUK_ERR_ERROR, "event_new error");
    }
    const struct timeval tv = {
        .tv_sec = 3600 * 24,
    };
    if (event_add(p->ev, &tv))
    {
        vm_finalizer_free(ctx, -1, iotjs_mtd_fd_free);
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
        duk_pop_3(ctx);
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "seek unknow whence %d", whence);
        break;
    }
    duk_pop_3(ctx);
    offset = lseek(p->fd, offset, whence);
    if (offset == -1)
    {
        int err = errno;
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    duk_push_number(ctx, offset);
    return 1;
}
static duk_ret_t native_erase_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    erase_info_t ei;
    ei.start = duk_require_number(ctx, 1);
    ei.length = duk_require_number(ctx, 2);
    if (ioctl(p->fd, MEMERASE, &ei) == -1)
    {
        int err = errno;
        duk_pop_3(ctx);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    return 0;
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
        int err = errno;
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
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
        int err = errno;
        duk_pop_2(ctx);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    duk_pop_2(ctx);
    duk_push_number(ctx, ok);
    return 1;
}
static void async_seek(vm_job_t *job)
{
    async_seek_t *in = job->in;
    async_out_t *out = job->out;
    out->ret = lseek(in->mtd->fd, in->offset, in->whence);
    if (out->ret == -1)
    {
        out->err = errno;
    }
    event_active(in->mtd->ev, 0, 0);
}
static duk_ret_t native_seek(duk_context *ctx)
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
        duk_pop_3(ctx);
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "seek unknow whence %d", whence);
        break;
    }

    vm_job_t *job = vm_new_job(ctx, sizeof(async_seek_t), sizeof(async_out_t));
    async_seek_t *in = job->in;
    in->mtd = p;
    in->offset = offset;
    in->whence = whence;

    p->job = job;
    vm_must_run_job(ctx, job, async_seek);
    return 0;
}
static void async_erase(vm_job_t *job)
{
    async_erase_t *in = job->in;
    async_out_t *out = job->out;
    erase_info_t ei;
    ei.start = in->start;
    ei.length = in->length;
    out->ret = ioctl(in->mtd->fd, MEMERASE, &ei);
    if (out->ret == -1)
    {
        out->err = errno;
    }
    event_active(in->mtd->ev, 0, 0);
}
static duk_ret_t native_erase(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_fd_free);
    iotjs_mtd_fd_t *p = finalizer->p;
    unsigned int start = duk_require_number(ctx, 1);
    unsigned int length = duk_require_number(ctx, 2);

    vm_job_t *job = vm_new_job(ctx, sizeof(async_erase_t), sizeof(async_out_t));
    async_erase_t *in = job->in;
    in->mtd = p;
    in->start = start;
    in->length = length;

    p->job = job;
    vm_must_run_job(ctx, job, async_erase);
    return 0;
}

static void async_read(vm_job_t *job)
{
    async_in_t *in = job->in;
    async_out_t *out = job->out;
    out->ret = read(in->mtd->fd, in->buf, in->sz);
    if (out->ret == -1)
    {
        out->err = errno;
    }
    event_active(in->mtd->ev, 0, 0);
}
static void async_write(vm_job_t *job)
{
    async_in_t *in = job->in;
    async_out_t *out = job->out;
    out->ret = write(in->mtd->fd, in->buf, in->sz);
    if (out->ret == -1)
    {
        out->err = errno;
    }
    event_active(in->mtd->ev, 0, 0);
}
static duk_ret_t native_read(duk_context *ctx)
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
    vm_job_t *job = vm_new_job(ctx, sizeof(async_in_t), sizeof(async_out_t));
    async_in_t *in = job->in;
    in->mtd = p;
    in->sz = sz;
    in->buf = buf;

    p->job = job;
    vm_must_run_job(ctx, job, async_read);
    return 0;
}
static duk_ret_t native_write(duk_context *ctx)
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
    vm_job_t *job = vm_new_job(ctx, sizeof(async_in_t), sizeof(async_out_t));
    async_in_t *in = job->in;
    in->mtd = p;
    in->sz = sz;
    in->buf = buf;

    p->job = job;
    vm_must_run_job(ctx, job, async_write);
    return 0;
}
typedef struct
{
    int fd;
    mtd_info_t mtd_info;
    duk_uint8_t state;
    struct event *ev;
    vm_job_t *job;

    spiffs *fs;
    u8_t *buf;
} iotjs_mtd_db_t;
static void iotjs_mtd_db_free(void *ptr)
{
#ifdef VM_TRACE_FINALIZER
    puts(" --------- iotjs_mtd_db_free");
#endif
    iotjs_mtd_db_t *p = ptr;
    if (p->fs)
    {
        SPIFFS_unmount(p->fs);
        p->fs = NULL;
    }
    if (p->buf)
    {
        vm_free(p->buf);
    }
    if (p->state)
    {
        p->state = 0;
        close(p->fd);
    }

    if (p->ev)
    {
        event_free(p->ev);
    }
}
static void iotjs_mtd_db_handler(evutil_socket_t fd, short events, void *args)
{
    if (events)
    {
        return;
    }
}
static duk_ret_t native_len(duk_context *ctx)
{
    duk_size_t sz;
    if (duk_is_string(ctx, 0))
    {
        duk_require_lstring(ctx, 0, &sz);
    }
    else
    {
        duk_require_buffer_data(ctx, 0, &sz);
    }
    duk_pop(ctx);
    duk_push_number(ctx, sz);
    return 1;
}
static duk_ret_t native_db(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    duk_uint_t device = duk_require_uint(ctx, 1);
    duk_require_callable(ctx, 2);
    duk_swap_top(ctx, 0);
    duk_pop_2(ctx);
    if (device > 2)
    {
        duk_error(ctx, DUK_ERR_ERROR, "unknow device %d", device);
    }
    // cb
    finalizer_t *finalizer = vm_create_finalizer_n(ctx, sizeof(iotjs_mtd_db_t));
    iotjs_mtd_db_t *p = finalizer->p;
    finalizer->free = iotjs_mtd_db_free;
    p->fd = open(path, O_RDWR);
    if (p->fd == -1)
    {
        int err = errno;
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    p->state = 0x1;
    if (ioctl(p->fd, MEMGETINFO, &p->mtd_info) == -1)
    {
        int err = errno;
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(p->fd, F_SETLK, &lock) == -1)
    {
        int err = errno;
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, strerror(err));
    }

    vm_context_t *vm = vm_get_context(ctx);
    p->ev = event_new(vm->eb, -1, EV_PERSIST | EV_TIMEOUT, iotjs_mtd_db_handler, p);
    if (!p->ev)
    {
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, "event_new error");
    }
    const struct timeval tv = {
        .tv_sec = 3600 * 24,
    };
    if (event_add(p->ev, &tv))
    {
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, "event_add error");
    }

    // mount
    spiffs_config cfg;
    cfg.phys_size = p->mtd_info.size;             // use all spi flash
    cfg.phys_addr = 0;                            // start spiffs at start of spi flash
    cfg.phys_erase_block = p->mtd_info.erasesize; // according to datasheet
    cfg.log_block_size = p->mtd_info.erasesize;   // let us not complicate things
    cfg.log_page_size = LOG_PAGE_SIZE;            // as we said

    if (p->mtd_info.writesize > 1)
    {
        p->buf = vm_malloc(p->mtd_info.writesize);
        if (!p->buf)
        {
            vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
            duk_error(ctx, DUK_ERR_ERROR, "vm_malloc buf fail");
        }
    }
    int res;
    switch (device)
    {
    case 0:
        IOTJS_SPIFFS_INIT(0, p->fd, p->mtd_info.writesize, p->buf, cfg);
        res = IOTJS_SPIFFS_MOUNT(0, &cfg);
        if (res < 0)
        {
            int err = IOTJS_SPIFFS_errno(0);
            vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
            duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
        }
        p->fs = IOTJS_SPIFFS_fs(0);
        break;
    case 1:
        IOTJS_SPIFFS_INIT(1, p->fd, p->mtd_info.writesize, p->buf, cfg);
        res = IOTJS_SPIFFS_MOUNT(1, &cfg);
        if (res < 0)
        {
            int err = IOTJS_SPIFFS_errno(1);
            vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
            duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
        }
        p->fs = IOTJS_SPIFFS_fs(1);
        break;
    default:
        IOTJS_SPIFFS_INIT(2, p->fd, p->mtd_info.writesize, p->buf, cfg);
        res = IOTJS_SPIFFS_MOUNT(2, &cfg);
        if (res < 0)
        {
            int err = IOTJS_SPIFFS_errno(2);
            vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
            duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
        }
        p->fs = IOTJS_SPIFFS_fs(2);
        break;
    }
    // cb, finalizer
    vm_snapshot_copy(ctx, VM_SNAPSHOT_MTD_KV, p, 2);
    return 1;
}
static duk_ret_t native_db_close(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *p = finalizer->p;
    if (p->fs)
    {
        SPIFFS_unmount(p->fs);
        p->fs = NULL;
    }
    if (p->state)
    {
        p->state = 0;
        close(p->fd);
    }
    return 0;
}
static duk_ret_t native_db_free(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    vm_remove_snapshot(ctx, VM_SNAPSHOT_MTD, finalizer->p);
    vm_finalizer_free(ctx, 0, iotjs_mtd_db_free);
    return 0;
}
static duk_ret_t native_key_encode(duk_context *ctx)
{
    duk_size_t sz;
    const void *src;
    if (duk_is_buffer_data(ctx, 0))
    {
        src = duk_get_buffer_data(ctx, 0, &sz);
    }
    else if (duk_is_string(ctx, 0))
    {
        src = duk_get_lstring(ctx, 0, &sz);
    }
    else
    {
        src = duk_safe_to_lstring(ctx, 0, &sz);
    }
    if (sz)
    {
        unsigned int len = iotjs_base64.raw_url.encoded_len(sz);
        void *dst = duk_push_buffer(ctx, len, 0);
        iotjs_base64.raw_url.encode(dst, src, sz);
        duk_buffer_to_string(ctx, -1);
    }
    else
    {
        duk_push_lstring(ctx, "", 0);
    }
    return 1;
}
// 讀取數據
// version 如果非 NULL，返回數據版本
// len 如果非 NULL, 返回數據長度
// cp 如果非 NULL, 將數據拷貝到此
static const char *spiffs_fs_get(spiffs *fs, const char *path, uint64_t *version, s32_t *len, void *cp)
{
    spiffs_file fd = SPIFFS_open(fs, path, SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        if (fd == SPIFFS_ERR_NOT_FOUND)
        {
            return 0;
        }
        return "SPIFFS_open fail";
    }
    spiffs_stat s;
    if (SPIFFS_fstat(fs, fd, &s) < 0)
    {
        SPIFFS_close(fs, fd);
        return "stat SPIFFS fail";
    }

    uint8_t buf[8];
    s32_t x = SPIFFS_read(fs, fd, buf, 4);
    if (x != 4)
    {
        SPIFFS_close(fs, fd);
        return "read SPIFFS short";
    }
    s32_t sz = iotjs_little_endian.uint32(buf);
    if (s.size != sz + 4 + 8)
    {
        SPIFFS_close(fs, fd);
        return "data corruption";
    }
    if (cp)
    {
        if (SPIFFS_read(fs, fd, cp, sz) != sz)
        {
            SPIFFS_close(fs, fd);
            return "read SPIFFS short";
        }
    }
    else
    {
        if (SPIFFS_lseek(fs, fd, sz, SPIFFS_SEEK_CUR) != sz + 4)
        {
            SPIFFS_close(fs, fd);
            return "lseek SPIFFS fail";
        }
    }
    if (SPIFFS_read(fs, fd, buf, 8) != 8)
    {
        SPIFFS_close(fs, fd);
        return "read SPIFFS short";
    }
    SPIFFS_close(fs, fd);
    if (version)
    {
        *version = iotjs_little_endian.uint64(buf);
    }
    if (len)
    {
        *len = sz;
    }
    return 0;
}
static duk_ret_t native_db_set_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *p = finalizer->p;
    spiffs *fs = p->fs;

    const char *emsg;

    const char *k0 = duk_require_string(ctx, 1);
    uint64_t v0 = 0;
    emsg = spiffs_fs_get(fs, k0, &v0, 0, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 5);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }
    const char *k1 = duk_require_string(ctx, 2);
    uint64_t v1 = 0;
    emsg = spiffs_fs_get(fs, k1, &v1, 0, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 5);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }

    duk_swap(ctx, 0, -2);
    duk_swap(ctx, 1, -1);
    duk_pop_3(ctx);
    uint64_t version;
    const char *k;
    const char *rm;
    if (v0 < v1)
    {
        k = k0;
        rm = k1;
        version = v1 + 1;
    }
    else
    {
        k = k1;
        rm = k0;
        version = v0 + 1;
    }

    duk_size_t sz_data;
    const u8_t *data;
    if (duk_is_string(ctx, 0))
    {
        data = duk_require_lstring(ctx, 0, &sz_data);
    }
    else
    {
        data = duk_require_buffer_data(ctx, 0, &sz_data);
    }
    duk_size_t sz;
    u8_t *buf = duk_require_buffer_data(ctx, 1, &sz);
    iotjs_little_endian.put_uint32(buf, sz_data);
    memcpy(buf + 4, data, sz_data);
    iotjs_little_endian.put_uint64(buf + 4 + sz_data, version);

    s32_t ok = SPIFFS_remove(fs, k);
    if (ok < 0 && ok != SPIFFS_ERR_NOT_FOUND)
    {
        int err = SPIFFS_errno(fs);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS_remove fail %d", err);
    }
    spiffs_file fd = SPIFFS_open(fs, k, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        int err = SPIFFS_errno(fs);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS_open fail %d", err);
    }
    if (SPIFFS_write(fs, fd, buf, sz) != sz)
    {
        int err = SPIFFS_errno(fs);
        SPIFFS_close(fs, fd);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS_write fail %d", err);
    }
    else
    {
        // success
        SPIFFS_remove(fs, rm);
        SPIFFS_close(fs, fd);
    }
    return 0;
}
static duk_ret_t native_db_has_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *p = finalizer->p;
    spiffs *fs = p->fs;

    const char *emsg;

    const char *k0 = duk_require_string(ctx, 1);
    uint64_t v0 = 0;
    emsg = spiffs_fs_get(fs, k0, &v0, 0, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 3);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }
    const char *k1 = duk_require_string(ctx, 2);
    uint64_t v1 = 0;
    emsg = spiffs_fs_get(fs, k1, &v1, 0, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 3);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }
    duk_pop_n(ctx, 3);
    if (v0 || v1)
    {
        duk_push_true(ctx);
    }
    else
    {
        duk_push_false(ctx);
    }
    return 1;
}

static duk_ret_t native_db_get_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *db = finalizer->p;
    spiffs *fs = db->fs;

    const char *emsg;

    const char *k0 = duk_require_string(ctx, 1);
    uint64_t v0 = 0;
    s32_t l0 = 0;
    emsg = spiffs_fs_get(fs, k0, &v0, &l0, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 4);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }

    const char *k1 = duk_require_string(ctx, 2);
    uint64_t v1 = 0;
    s32_t l1 = 0;
    emsg = spiffs_fs_get(fs, k1, &v1, &l1, 0);
    if (emsg)
    {
        duk_pop_n(ctx, 4);
        duk_error(ctx, DUK_ERR_ERROR, emsg);
    }
    duk_bool_t s = duk_require_boolean(ctx, 3);
    duk_pop_n(ctx, 4);

    if (!v0 && !v1)
    {
        return 0;
    }
    const char *k;
    s32_t l;
    if (v0 < v1)
    {
        k = k1;
        l = l1;
    }
    else
    {
        k = k0;
        l = l0;
    }

    if (l)
    {
        void *dst = duk_push_buffer(ctx, l, 0);
        emsg = spiffs_fs_get(fs, k, 0, 0, dst);
        if (emsg)
        {
            duk_error(ctx, DUK_ERR_ERROR, emsg);
        }
        if (s)
        {
            duk_buffer_to_string(ctx, -1);
        }
    }
    else
    {
        if (s)
        {
            duk_push_lstring(ctx, "", 0);
        }
        else
        {
            duk_push_buffer(ctx, 0, 0);
        }
    }
    return 1;
}
static duk_ret_t native_db_info(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *db = finalizer->p;
    spiffs *fs = db->fs;

    duk_pop(ctx);

    duk_push_object(ctx);
    duk_push_number(ctx, fs->free_blocks);
    duk_put_prop_lstring(ctx, -2, "freeBlocks", 10);
    duk_push_number(ctx, fs->block_count);
    duk_put_prop_lstring(ctx, -2, "blockCount", 10);
    duk_push_number(ctx, (u32_t)SPIFFS_OBJ_IX_LEN(fs));
    duk_put_prop_lstring(ctx, -2, "entries", 7);
    duk_push_number(ctx, (u32_t)SPIFFS_OBJ_HDR_IX_LEN(fs));
    duk_put_prop_lstring(ctx, -2, "headerEntries", 13);

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
        duk_push_c_lightfunc(ctx, native_erase_sync, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "eraseSync", 9);
        duk_push_c_lightfunc(ctx, native_read_sync, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "readSync", 8);
        duk_push_c_lightfunc(ctx, native_write_sync, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "writeSync", 9);

        duk_push_c_lightfunc(ctx, native_seek, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "seek", 4);
        duk_push_c_lightfunc(ctx, native_erase, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "erase", 5);
        duk_push_c_lightfunc(ctx, native_read, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "read", 4);
        duk_push_c_lightfunc(ctx, native_write, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "write", 5);

        duk_push_c_lightfunc(ctx, native_len, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "len", 3);

        duk_push_c_lightfunc(ctx, native_db, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "db", 2);
        duk_push_c_lightfunc(ctx, native_db_close, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "db_close", 8);
        duk_push_c_lightfunc(ctx, native_db_free, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "db_free", 7);
        duk_push_c_lightfunc(ctx, native_key_encode, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "key_encode", 10);

        duk_push_c_lightfunc(ctx, native_db_set_sync, 5, 5, 0);
        duk_put_prop_lstring(ctx, -2, "db_set_sync", 11);
        duk_push_c_lightfunc(ctx, native_db_has_sync, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "db_has_sync", 11);
        duk_push_c_lightfunc(ctx, native_db_get_sync, 4, 4, 0);
        duk_put_prop_lstring(ctx, -2, "db_get_sync", 11);
        duk_push_c_lightfunc(ctx, native_db_info, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "db_info", 7);
    }
    duk_call(ctx, 3);
    return 0;
}