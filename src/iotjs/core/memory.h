#ifndef IOTJS_CORE_MEMORY_H
#define IOTJS_CORE_MEMORY_H
#include <stddef.h>
typedef void *(*vm_malloc_t)(size_t sz);
typedef void *(*vm_realloc_t)(void *ptr, size_t sz);
typedef void (*vm_free_t)(void *ptr);

void *vm_malloc(size_t sz);
void *vm_realloc(void *ptr, size_t sz);
void vm_free(void *ptr);
void vm_set_mem_functions(vm_malloc_t malloc, vm_realloc_t realloc, vm_free_t free);
void vm_set_mem_default_pool(size_t size);
#endif