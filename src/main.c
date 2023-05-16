#include <iotjs/core/vm.h>
#include <iotjs/core/memory.h>
#include <event2/thread.h>
#include <event2/event.h>
int main(int argc, char *argv[])
{
    if (evthread_use_pthreads())
    {
        puts("evthread_use_pthreads error");
        return -1;
    }

    vm_memory_init(200);
    event_set_mem_functions(vm_libevent_malloc, vm_libevent_realloc, vm_libevent_free);

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
    if (vm_main(ctx, filename, argc, argv))
    {
        printf("iotjs_main: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        goto EXIT_ERROR;
    }
    duk_pop(ctx);
    vm_context_t *vm = vm_get_context(ctx);
    int err = event_base_dispatch(vm->eb);
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