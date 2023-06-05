#include <iotjs/core/memory.h>
#include <stdlib.h>

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