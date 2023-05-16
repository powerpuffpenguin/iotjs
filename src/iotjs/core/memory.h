#ifndef IOTJS_CORE_MEMORY_H
#define IOTJS_CORE_MEMORY_H

#include <stddef.h>
void vm_memory_init(int events);
void *vm_libevent_malloc(size_t sz);
void *vm_libevent_realloc(void *ptr, size_t sz);
void vm_libevent_free(void *ptr);
#endif