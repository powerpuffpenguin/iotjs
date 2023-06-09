#include <stddef.h>
static unsigned char iotjs_c_uper(unsigned char c)
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

    unsigned char l, r;
    for (size_t i = 0; i < n; i++)
    {
        l = iotjs_c_uper(*p1++);
        r = iotjs_c_uper(*p2++);
        if (l == r)
        {
            continue;
        }
        return l < r ? -1 : 1;
    }
    return 0;
}