#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iotjs/mempool/mempool.h>

void init_mep(iotjs_mep_t *mep, iotjs_mep_alloctor_t *alloctors, size_t *cache, size_t len)
{
    size_t block = 16 / 2; // 小於 16 的內存分配直接分配 16 字節
    size_t num = 0;
    for (size_t i = 0; i < len; i++)
    {
        block *= 2;
        alloctors[i].block = block;
        alloctors[i].cache = cache[i];
        iotjs_mep_list_init(&alloctors[i].idle);
        iotjs_mep_list_init(&alloctors[i].used);
        printf("block=%zu count=%zu cache=%zuk\r\n", block, alloctors[i].cache, block * alloctors[i].cache / 1024);
        num += block * alloctors[i].cache;
    }
    printf("total=%zuk\r\n", num / 1024);

    iotjs_mep_init(mep, alloctors, len);
}
int main(int argc, char *argv[])
{
    int ret = -1;
    iotjs_mep_t mep;
    iotjs_mep_alloctor_t alloctors[13];
    size_t cache[13] = {
        8192,
        4096,
        2048,
        1024,
        512,
        256,
        128,
        64,
        32,
        16,
        8,
        8,
        8,
    };
    init_mep(&mep, alloctors, cache, sizeof(alloctors) / sizeof(iotjs_mep_alloctor_t));

    void *s = iotjs_mep_malloc(&mep, 1024 * 128);
    if (!s)
    {
        puts("iotjs_mep_malloc 128k fail");
        goto END;
    }
    iotjs_mep_free(&mep, s);
    size_t x = 1;
    for (size_t i = 0; i < 16; i++)
    {
        x *= 2;
        printf("iotjs_mep_malloc(%zu)\r\n", x);
        s = iotjs_mep_malloc(&mep, x);
        if (!s)
        {
            printf("iotjs_mep_malloc %zu fail\r\n", x);
            goto END;
        }
        printf("iotjs_mep_free(%zu)\r\n", x);
        iotjs_mep_free(&mep, s);
    }

    ret = 0;
END:
    iotjs_mep_destroy(&mep);
    return ret;
}