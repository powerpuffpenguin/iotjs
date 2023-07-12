#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iotjs/mempool/mempool.h>

int main(int argc, char *argv[])
{
    int ret = -1;
    iotjs_mep_t *mep = iotjs_mep_new();
    if (!mep)
    {
        puts("iotjs_mep_new fail");
        return ret;
    }

    void *s = iotjs_mep_malloc(mep, 1024 * 128);
    if (!s)
    {
        puts("iotjs_mep_malloc 128k fail");
        goto END;
    }
    iotjs_mep_free(mep, s);
    size_t x = 1;
    for (size_t i = 0; i < 16; i++)
    {
        x *= 2;
        s = iotjs_mep_malloc(mep, x);
        if (!s)
        {
            printf("iotjs_mep_malloc %zu fail\r\n", x);
            goto END;
        }
        iotjs_mep_free(mep, s);
    }

    ret = 0;
END:
    iotjs_mep_destroy(mep);
    return ret;
}