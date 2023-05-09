#ifndef IOTJS_CORE_PATH_H
#define IOTJS_CORE_PATH_H
#include <stddef.h>
typedef struct
{
    char *p;
    size_t cap;
    size_t offset;
    size_t len;
    size_t reference;
} path_string_t;

path_string_t path_clean(const char *path, size_t size);
#endif