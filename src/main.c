#include <iotjs/core/vm.h>
#include <iotjs/core/core.h>
#include <iotjs/core/memory.h>
#include <event2/thread.h>
#include <event2/event.h>
void *alloc_function(void *udata, duk_size_t size)
{
    return vm_malloc(size);
}
void *realloc_function(void *udata, void *ptr, duk_size_t size)
{
    return vm_realloc(ptr, size);
}
void free_function(void *udata, void *ptr)
{
    vm_free(ptr);
}
int main(int argc, char *argv[])
{
    int ret = -1;
    if (evthread_use_pthreads())
    {
        puts("evthread_use_pthreads error");
        return ret;
    }
    event_set_mem_functions(vm_malloc, vm_realloc, vm_free);
    char *filename;
    if (argc >= 2)
    {
        filename = argv[1];
    }
    else
    {
        filename = "main.js";
    }
    duk_context *ctx = duk_create_heap(alloc_function, realloc_function, free_function, NULL, NULL);
    if (!ctx)
    {
        puts("duk_create_heap error");
        return ret;
    }
    vm_init_core();
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
    ret = 0;
EXIT_ERROR:
    duk_destroy_heap(ctx);
    return ret;
}