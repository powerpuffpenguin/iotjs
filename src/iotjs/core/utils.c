#include <stddef.h>
unsigned char iotjs_c_lower(unsigned char c)
{
    if (c >= 'a' && c <= 'z')
    {
        c += 'A' - 'a';
    }
    return c;
}
int iotjs_memcasecmp(const void *s1, const void *s2, size_t n)
{
    if (n == 0 || s1 == s2)
    {
        return 0;
    }

    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    int result;
    for (size_t i = 0; i < n; i++)
    {
        result = iotjs_c_lower(*p1++) - iotjs_c_lower(*p2++);
        if (result)
        {
            return result;
        }
    }
    return 0;
}