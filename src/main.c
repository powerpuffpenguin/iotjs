#include <iotjs/core/vm.h>

int main(int argc, char *argv[])
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
    if (vm_main(ctx, filename))
    {
        printf("iotjs_main: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }
    duk_pop(ctx);
    event_base_t *eb = vm_event_base(ctx);
    if (!eb)
    {
        printf("iotjs_loop: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }
    int err = event_base_dispatch(eb);
    if (err < 0)
    {
        goto EXIT_ERROR;
    }

    duk_destroy_heap(ctx);
    return 0;
EXIT_ERROR:
    duk_destroy_heap(ctx);
    return -1;
}