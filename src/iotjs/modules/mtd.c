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

#include <spiffs.h>

static spiffs mtd_fs_spiffs;
#define LOG_PAGE_SIZE 256
static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];
int mtd_fs_fd = -1;

static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    // printf("my_spiffs_read %d %d \r\n", addr, size);
    int ret = lseek(mtd_fs_fd, addr, SEEK_SET);
    if (ret == -1)
    {
        return ret;
    }
    ret = read(mtd_fs_fd, dst, size);
    if (ret == -1)
    {
        return ret;
    }
    return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
    // printf("my_spiffs_write %d %d \r\n", addr, size);
    int ret = lseek(mtd_fs_fd, addr, SEEK_SET);
    if (ret == -1)
    {
        return ret;
    }
    ret = write(mtd_fs_fd, src, size);
    if (ret == -1)
    {
        return ret;
    }
    return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size)
{
    printf("my_spiffs_erase %d %d \r\n", addr, size);
    erase_info_t ei;
    ei.start = addr;
    ei.length = size;
    int ret = ioctl(mtd_fs_fd, MEMERASE, &ei);
    if (ret == -1)
    {
        return ret;
    }
    return SPIFFS_OK;
}

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
    duk_swap_top(ctx, -2);
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
duk_ret_t native_db(duk_context *ctx)
{
    const char *path = duk_require_string(ctx, 0);
    duk_require_callable(ctx, 1);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);

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

    mtd_fs_fd = p->fd;

    // mount
    spiffs_config cfg;
    cfg.phys_size = p->mtd_info.size;             // use all spi flash
    cfg.phys_addr = 0;                            // start spiffs at start of spi flash
    cfg.phys_erase_block = p->mtd_info.erasesize; // according to datasheet
    cfg.log_block_size = p->mtd_info.erasesize;   // let us not complicate things
    cfg.log_page_size = LOG_PAGE_SIZE;            // as we said

    cfg.hal_read_f = my_spiffs_read;
    cfg.hal_write_f = my_spiffs_write;
    cfg.hal_erase_f = my_spiffs_erase;

    int res = SPIFFS_mount(&mtd_fs_spiffs,
                           &cfg,
                           spiffs_work_buf,
                           spiffs_fds,
                           sizeof(spiffs_fds),
                           spiffs_cache_buf,
                           sizeof(spiffs_cache_buf),
                           0);
    if (res < 0)
    {
        int err = SPIFFS_errno(&mtd_fs_spiffs);
        vm_finalizer_free(ctx, -1, iotjs_mtd_db_free);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
    }
    p->fs = &mtd_fs_spiffs;
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
static duk_ret_t native_base64_encode(duk_context *ctx)
{
    duk_base64_encode(ctx, -1);
    return 1;
}

static duk_ret_t native_db_set_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *p = finalizer->p;
    spiffs *fs = p->fs;
    const char *key = duk_require_string(ctx, 1);
    duk_size_t sz;
    u8_t *buf = duk_require_buffer_data(ctx, 2, &sz);

    spiffs_file fd = SPIFFS_open(fs, key, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        int err = SPIFFS_errno(fs);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
    }
    if (SPIFFS_write(fs, fd, buf, sz) < 0)
    {
        int err = SPIFFS_errno(fs);
        SPIFFS_close(fs, fd);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
    }
    else
    {
        SPIFFS_close(fs, fd);
    }
    return 0;
}
typedef struct
{
    spiffs *fs;
    spiffs_file fd;
} spiffs_file_fd_t;
static void native_spiffs_file_free(void *args)
{
    spiffs_file_fd_t *fd = args;
    SPIFFS_close(fd->fs, fd->fd);
}
static duk_ret_t native_db_get_sync(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, iotjs_mtd_db_free);
    iotjs_mtd_db_t *db = finalizer->p;
    duk_swap_top(ctx, 0);
    duk_pop(ctx);
    const char *key = duk_require_string(ctx, 0);

    finalizer = vm_create_finalizer_n(ctx, sizeof(spiffs_file_fd_t));
    spiffs_file_fd_t *p = finalizer->p;
    p->fs = db->fs;
    p->fd = SPIFFS_open(p->fs, key, SPIFFS_RDWR, 0);
    if (p->fd < 0)
    {
        if (p->fd == SPIFFS_ERR_NOT_FOUND)
        {
            return 0;
        }
        int err = SPIFFS_errno(p->fs);
        duk_error(ctx, DUK_ERR_ERROR, "SPIFFS fail %d", err);
    }
    finalizer->free = native_spiffs_file_free;

    spiffs_stat s;
    if (SPIFFS_fstat(p->fs, p->fd, &s) < 0)
    {
        int err = SPIFFS_errno(p->fs);
        vm_finalizer_free(ctx, -1, native_spiffs_file_free);
        duk_error(ctx, DUK_ERR_ERROR, "stat SPIFFS fail %d", err);
    }
    void *buf = duk_push_buffer(ctx, s.size, 0);
    s32_t x = SPIFFS_read(p->fs, p->fd, buf, s.size);
    if (x < 0)
    {
        int err = SPIFFS_errno(p->fs);
        vm_finalizer_free(ctx, -2, native_spiffs_file_free);
        duk_error(ctx, DUK_ERR_ERROR, "read SPIFFS fail %d", err);
    }
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

        duk_push_c_lightfunc(ctx, native_db, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "db", 2);
        duk_push_c_lightfunc(ctx, native_db_close, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "db_close", 8);
        duk_push_c_lightfunc(ctx, native_db_free, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "db_free", 7);
        duk_push_c_lightfunc(ctx, native_base64_encode, 1, 1, 0);
        duk_put_prop_lstring(ctx, -2, "base64_encode", 13);

        duk_push_c_lightfunc(ctx, native_db_set_sync, 3, 3, 0);
        duk_put_prop_lstring(ctx, -2, "db_set_sync", 11);
        duk_push_c_lightfunc(ctx, native_db_get_sync, 2, 2, 0);
        duk_put_prop_lstring(ctx, -2, "db_get_sync", 11);
    }
    duk_call(ctx, 3);
    return 0;
}