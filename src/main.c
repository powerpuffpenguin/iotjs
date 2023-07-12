#include <iotjs/core/vm.h>
#include <iotjs/core/core.h>
#include <iotjs/core/memory.h>
#include <event2/thread.h>
#include <event2/event.h>
#include <iotjs/mempool/mempool.h>
#include <pthread.h>
typedef struct
{
    iotjs_mep_t mep;
    iotjs_mep_alloctor_t alloctors[13];
    pthread_mutex_t mutex;
} default_mep_t;
default_mep_t default_mep;
void init_default_mep()
{
    if (pthread_mutex_init(&default_mep.mutex, NULL))
    {
        puts("init_default_mep_mutex fail");
        exit(1);
    }

    iotjs_mep_t *mep = &default_mep.mep;
    iotjs_mep_alloctor_t *alloctors = (void *)&default_mep.alloctors;

    size_t len = 13;
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
    size_t block = 16 / 2; // 小於 16 的內存分配直接分配 16 字節
    size_t num = 0;
    for (size_t i = 0; i < len; i++)
    {
        block *= 2;
        alloctors[i].block = block;
        alloctors[i].cache = cache[i];
        iotjs_mep_list_init(&alloctors[i].idle);
        iotjs_mep_list_init(&alloctors[i].used);
    }
    iotjs_mep_init(mep, alloctors, len);
}

void *my_malloc(size_t sz)
{
    pthread_mutex_lock(&default_mep.mutex);
    void *p = iotjs_mep_malloc(&default_mep.mep, sz);
    pthread_mutex_unlock(&default_mep.mutex);
    return p;
}
void *my_realloc(void *p, size_t sz)
{
    pthread_mutex_lock(&default_mep.mutex);
    p = iotjs_mep_realloc(&default_mep.mep, p, sz);
    pthread_mutex_unlock(&default_mep.mutex);
    return p;
}
void my_free(void *p)
{
    if (p)
    {
        pthread_mutex_lock(&default_mep.mutex);
        iotjs_mep_free(&default_mep.mep, p);
        pthread_mutex_unlock(&default_mep.mutex);
    }
}
void *alloc_function(void *udata, duk_size_t sz)
{
    pthread_mutex_lock(&default_mep.mutex);
    void *p = iotjs_mep_malloc(&default_mep.mep, sz);
    pthread_mutex_unlock(&default_mep.mutex);
    return p;
}
void *realloc_function(void *udata, void *p, duk_size_t sz)
{
    pthread_mutex_lock(&default_mep.mutex);
    p = iotjs_mep_realloc(&default_mep.mep, p, sz);
    pthread_mutex_unlock(&default_mep.mutex);
    return p;
}
void free_function(void *udata, void *p)
{
    if (p)
    {
        pthread_mutex_lock(&default_mep.mutex);
        iotjs_mep_free(&default_mep.mep, p);
        pthread_mutex_unlock(&default_mep.mutex);
    }
}

int main(int argc, char *argv[])
{
    init_default_mep();
    int ret = -1;
    if (evthread_use_pthreads())
    {
        puts("evthread_use_pthreads error");
        return ret;
    }
    event_set_mem_functions(my_malloc, my_realloc, my_free);
    vm_set_mem_functions(my_malloc, my_realloc, my_free);
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