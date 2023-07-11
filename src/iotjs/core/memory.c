#include <iotjs/core/memory.h>
#include <stdlib.h>
#include <mem_pool.h>
#include <string.h>

vm_malloc_t __vm_iotjs_malloc = malloc;
vm_realloc_t __vm_iotjs_realloc = realloc;
vm_free_t __vm_iotjs_free = free;
void *vm_malloc(size_t sz)
{
    return __vm_iotjs_malloc(sz);
}
void *vm_realloc(void *ptr, size_t sz)
{
    return __vm_iotjs_realloc(ptr, sz);
}
void vm_free(void *ptr)
{
    __vm_iotjs_free(ptr);
}
void vm_set_mem_functions(vm_malloc_t malloc, vm_realloc_t realloc, vm_free_t free)
{
    __vm_iotjs_malloc = malloc;
    __vm_iotjs_realloc = realloc;
    __vm_iotjs_free = free;
}

static MemPool *__vm_iotjs_mp = NULL;
static void *mp_malloc(size_t sz)
{
    char *chunk = Malloc(__vm_iotjs_mp, sz + sizeof(size_t));
    // char *chunk = malloc(sz + sizeof(size_t));
    if (!chunk)
        return chunk;
    *(size_t *)chunk = sz;
    return chunk + sizeof(size_t);
}
static void mp_free(void *ptr)
{
    ptr = (char *)ptr - sizeof(size_t);
    Free(__vm_iotjs_mp, ptr);
    // free(ptr);
}
static void *mp_realloc(void *ptr, size_t sz)
{
    if (!ptr)
    {
        return mp_malloc(sz);
    }
    else if (!sz)
    {
        mp_free(ptr);
        return NULL;
    }

    size_t old_size = *(size_t *)((char *)ptr - sizeof(size_t));
    void *dst = mp_malloc(sz);
    if (dst)
    {
        memcpy(dst, ptr, old_size < sz ? old_size : sz);
    }
    mp_free(ptr);
    return dst;
}

void vm_set_mem_default_pool(size_t size)
{
    if (!__vm_iotjs_mp)
    {
        __vm_iotjs_mp = NewMemPool(size);
    }

    __vm_iotjs_malloc = mp_malloc;
    __vm_iotjs_realloc = mp_realloc;
    __vm_iotjs_free = mp_free;
}