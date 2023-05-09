#ifndef IOTJS_CORE_STRINGS_H
#define IOTJS_CORE_STRINGS_H
#include <stddef.h>
#include <iotjs/core/defines.h>

typedef struct
{
    char *p;
    size_t reference;
} strings_reference_t;
typedef struct
{
    size_t cap;
    size_t len;
    size_t offset;
} strings_metadata_t;
typedef struct
{
    strings_reference_t *reference;
    strings_metadata_t metadata;
} strings_t;

BOOL strings_new(strings_t *s, size_t len);
BOOL strings_new_cap(strings_t *s, size_t len, size_t cap);
strings_t *append(strings_t *s, strings_t *o);

#endif