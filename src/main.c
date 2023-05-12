#include <sys/epoll.h>

int main(int argc, char *argv[])
{
    epoll_create1(0);
    return 0;
}

#include <iotjs/core/vm.h>

int main1(int argc, char *argv[])
{
    char *filename;
    if (argc >= 2)
    {
        filename = argv[1];
    }
    else
    {
        filename = "main.js";
    }

    duk_context *ctx = duk_create_heap_default();
    if (!ctx)
    {
        puts("duk_create_heap_default error");
        return -1;
    }
    duk_ret_t err = vm_main(ctx, filename);
    if (err)
    {
        printf("iotjs_main: %s\n", duk_safe_to_string(ctx, -1));
    }
    else
    {
        duk_pop(ctx);
    }
    duk_destroy_heap(ctx);
    return err;
}