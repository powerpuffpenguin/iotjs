#include <event2/buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int ret = -1;
    struct evbuffer_ptr pos;
    pos.pos = -2;
    if (pos.pos < 0)
    {
        puts("ok");
        printf("%ld\n", pos.pos);
    }
    else
    {
        printf("%ld\n", pos.pos);
    }
END:
    return ret;
}