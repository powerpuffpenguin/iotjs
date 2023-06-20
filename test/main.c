#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int example_mtd(int fd)
{
    // mtd_info_t 記錄了一些 設備相關的信息
    mtd_info_t mtd_info;
    if (ioctl(fd, MEMGETINFO, &mtd_info) == -1)
    {
        printf("ioctl MEMGETINFO: %s\n", strerror(errno));
        return -1;
    }
    printf("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
           mtd_info.type, mtd_info.size, mtd_info.erasesize);

    // 擦除數據，每次寫入數據前需要對塊進行擦除否則可能會寫入錯誤的數據
    erase_info_t ei;
    ei.length = mtd_info.erasesize; // set the erase block size
    for (ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        if (ioctl(fd, MEMERASE, &ei) == -1)
        {
            printf("ioctl MEMERASE: %s\n", strerror(errno));
            return -1;
        }
    }

    // 檢測壞塊，擦除數據後塊內容應該全部是 0xff 通過檢測是否全 0xff 來判斷是否是一個損壞的塊
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf("lseek: %s\n", strerror(errno));
        return -1;
    }
    char *block = malloc(ei.length * 2);
    memset(block + ei.length, 0xff, ei.length);
    for (ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        if (read(fd, block, ei.length) == -1)
        {
            printf("read: %s\n", strerror(errno));
            return -1;
        }
        if (memcmp(block, block + ei.length, ei.length))
        {
            printf("BAD block %x %x\n", ei.start, ei.length);
            return -1;
        }
    }

    // 寫入數據
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf("lseek: %s\n", strerror(errno));
        return -1;
    }
    const char *str = "cerberus is an idea";
    memcpy(block, str, strlen(str));
    if (write(fd, block, ei.length) == -1) // 必須寫入一個完整的塊
    {
        printf("write: %s\n", strerror(errno));
        return -1;
    }

    // 讀取數據
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf("lseek: %s\n", strerror(errno));
        return -1;
    }
    ssize_t n = read(fd, block + ei.length, strlen(str));
    if (n == -1)
    {
        printf("read: %s\n", strerror(errno));
        return -1;
    }
    printf("read: %.*s\n", n, block + ei.length);
    /* code */
    return 0;
}

int main(int argc, char *argv[])
{
    // 打開設備進行讀寫
    int fd = open("/dev/mtd0", O_RDWR);
    if (fd == -1)
    {
        puts(strerror(errno));
        return -1;
    }
    int ok = example_mtd(fd);

    // 關閉設備
    close(fd);
    return ok;
}