#include <iotjs/core/vm.h>
#include <event2/thread.h>
#include <event2/event.h>
int main(int argc, char *argv[])
{
    if (evthread_use_pthreads())
    {
        puts("evthread_use_pthreads error");
        return -1;
    }

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
    if (vm_init(ctx, argc, argv))
    {
        printf("iotjs_init: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }
    if (vm_main(ctx, filename))
    {
        printf("iotjs_main: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }
    duk_pop(ctx);
    if (vm_loop(ctx))
    {
        printf("iotjs_loop: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }

    duk_destroy_heap(ctx);
    return 0;
EXIT_ERROR:
    duk_destroy_heap(ctx);
    return -1;
}