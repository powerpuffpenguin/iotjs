#include <iotjs/core/strings.h>
#include <memory.h>
#include <stdlib.h>
BOOL strings_new(strings_t *s, size_t len)
{
    strings_t s;
    if (len == 0)
    {
        memset(&s, 0, sizeof(strings_t));
        return TRUE;
    }
    strings_reference_t *reference = (strings_reference_t *)malloc(sizeof(strings_reference_t) + len);
    if (!reference)
    {
        return FALSE;
    }
    s->reference = reference;
    reference->reference = 1;
    reference->p = ((char *)reference) + sizeof(strings_reference_t);
    memset(&s->metadata, 0, sizeof(strings_metadata_t));
    return TRUE;
}
BOOL strings_new_cap(strings_t *s, size_t len, size_t cap)
{
    if (cap < len)
    {
        cap = len;
    }
    BOOL ok = strings_new(s, cap);
    if (ok)
    {
        s->metadata.len = len;
    }
    return ok;
}