#ifndef IOTJS_CORE_UTILS_H
#define IOTJS_CORE_UTILS_H
#include <stddef.h>
// s1< s2 ? -1 : (s1 == s2 ? 0 : 1)
int iotjs_memcasecmp(const void *s1, const void *s2, size_t n);
#endif