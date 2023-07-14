#ifndef IOTJS_MODULES_MTD_DB_H
#define IOTJS_MODULES_MTD_DB_H

#include <spiffs.h>

#define LOG_PAGE_SIZE 256

typedef struct
{
    spiffs spiffs;
    u8_t work_buf[LOG_PAGE_SIZE * 2];
    u8_t fds[32 * 4];
    u8_t cache_buf[(LOG_PAGE_SIZE + 32) * 4];

    int fd;
    u32_t writesize;
    u8_t *buf;
} iotjs_spiffs_fs_t;

#define IOTJS_SPIFFS_DEFINE(x)                                              \
    static iotjs_spiffs_fs_t __iotjs_spiffs_fs##x = {.fd = -1};             \
    static s32_t __iotjs_spiffs_read##x(u32_t addr, u32_t size, u8_t *dst)  \
    {                                                                       \
        int ret = lseek(__iotjs_spiffs_fs##x.fd, addr, SEEK_SET);           \
        if (ret == -1)                                                      \
        {                                                                   \
            return ret;                                                     \
        }                                                                   \
        ret = read(__iotjs_spiffs_fs##x.fd, dst, size);                     \
        if (ret == -1)                                                      \
        {                                                                   \
            return ret;                                                     \
        }                                                                   \
        return SPIFFS_OK;                                                   \
    }                                                                       \
    static s32_t __iotjs_spiffs_erase##x(u32_t addr, u32_t size)            \
    {                                                                       \
        erase_info_t ei;                                                    \
        ei.start = addr;                                                    \
        ei.length = size;                                                   \
        int ret = ioctl(__iotjs_spiffs_fs##x.fd, MEMERASE, &ei);            \
        if (ret == -1)                                                      \
        {                                                                   \
            return ret;                                                     \
        }                                                                   \
        return SPIFFS_OK;                                                   \
    }                                                                       \
    static s32_t __iotjs_spiffs_write##x(u32_t addr, u32_t size, u8_t *src) \
    {                                                                       \
        u32_t writesize = __iotjs_spiffs_fs##x.writesize;                   \
        if (size)                                                           \
        {                                                                   \
            int fd = __iotjs_spiffs_fs##x.fd;                               \
            if (writesize > 1)                                              \
            {                                                               \
                u8_t *buf = __iotjs_spiffs_fs##x.buf;                       \
                u32_t offset = addr / writesize * writesize;                \
                int ret = lseek(fd, offset, SEEK_SET);                      \
                if (ret == -1)                                              \
                {                                                           \
                    return ret;                                             \
                }                                                           \
                u32_t n = addr - offset;                                    \
                if (n)                                                      \
                {                                                           \
                    ret = read(fd, buf, writesize);                         \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                    u32_t cp = writesize - n;                               \
                    if (cp > size)                                          \
                    {                                                       \
                        cp = size;                                          \
                    }                                                       \
                    memcpy(buf + n, src, cp);                               \
                    src += cp;                                              \
                    size -= cp;                                             \
                    int ret = lseek(fd, offset, SEEK_SET);                  \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                    ret = write(fd, buf, writesize);                        \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                    offset += writesize;                                    \
                }                                                           \
                n = size / writesize * writesize;                           \
                ret = write(fd, src, n);                                    \
                if (ret == -1)                                              \
                {                                                           \
                    return ret;                                             \
                }                                                           \
                size -= n;                                                  \
                if (size)                                                   \
                {                                                           \
                    src += n;                                               \
                    offset += n;                                            \
                                                                            \
                    ret = read(fd, buf, writesize);                         \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                    ret = lseek(fd, offset, SEEK_SET);                      \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                    memcpy(buf, src, size);                                 \
                    ret = write(fd, buf, writesize);                        \
                    if (ret == -1)                                          \
                    {                                                       \
                        return ret;                                         \
                    }                                                       \
                }                                                           \
            }                                                               \
            else                                                            \
            {                                                               \
                int ret = lseek(fd, addr, SEEK_SET);                        \
                if (ret == -1)                                              \
                {                                                           \
                    return ret;                                             \
                }                                                           \
                ret = write(fd, src, size);                                 \
                if (ret == -1)                                              \
                {                                                           \
                    return ret;                                             \
                }                                                           \
            }                                                               \
        }                                                                   \
        return SPIFFS_OK;                                                   \
    }

#define IOTJS_SPIFFS_INIT(x, _fd, _writesize, _buf, cfg) \
    {                                                    \
        __iotjs_spiffs_fs##x.fd = _fd;                   \
        __iotjs_spiffs_fs##x.writesize = _writesize;     \
        __iotjs_spiffs_fs##x.buf = _buf;                 \
        cfg.hal_read_f = __iotjs_spiffs_read##x;         \
        cfg.hal_write_f = __iotjs_spiffs_write##x;       \
        cfg.hal_erase_f = __iotjs_spiffs_erase##x;       \
    }
#define IOTJS_SPIFFS_MOUNT(x, cfg) SPIFFS_mount(&__iotjs_spiffs_fs##x.spiffs,           \
                                                cfg,                                    \
                                                __iotjs_spiffs_fs##x.work_buf,          \
                                                __iotjs_spiffs_fs##x.fds,               \
                                                sizeof(__iotjs_spiffs_fs##x.fds),       \
                                                __iotjs_spiffs_fs##x.cache_buf,         \
                                                sizeof(__iotjs_spiffs_fs##x.cache_buf), \
                                                0)
#define IOTJS_SPIFFS_errno(x) SPIFFS_errno(&__iotjs_spiffs_fs##x.spiffs);

#define IOTJS_SPIFFS_fs(x) (&__iotjs_spiffs_fs##x.spiffs);
#endif
