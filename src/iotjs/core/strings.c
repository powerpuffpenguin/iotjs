#include <iotjs/core/strings.h>
#include <memory.h>
#include <stdlib.h>

string_t strings_make(size_t len)
{
    return strings_make_cap(len, len);
}
string_t strings_make_cap(size_t len, size_t cap)
{
    if (cap < len)
    {
        cap = len;
    }
    if (cap == 0)
    {
        IOTJS_VAR_STRUCT(string_t, s);
        return s;
    }
    strings_reference_t *reference = (strings_reference_t *)malloc(sizeof(strings_reference_t) + cap);
    if (!reference)
    {
        IOTJS_VAR_STRUCT(string_t, s);
        return s;
    }

    reference->count = 1;
    reference->cap = cap;
    if (cap > 0)
    {
        reference->ptr = ((char *)reference) + sizeof(strings_reference_t);
        memset(reference->ptr, 0, cap);
    }
    else
    {
        reference->ptr = NULL;
    }
    string_t s = {
        .len = len,
        .offset = 0,
        .reference = reference,
    };
    return s;
}
string_t strings_from_c_str(const char *s)
{
    size_t cap = strlen(s);
    string_t str = strings_make_cap(cap, cap);
    if (IOTJS_REFERENCE_VALID(str) && cap)
    {
        memcpy(IOTJS_REFERENCE_POINTER(str), s, cap);
    }
    return str;
}
string_t strings_from_str(const char *s, size_t n)
{
    string_t str = strings_make_cap(n, n);
    if (IOTJS_REFERENCE_VALID(str) && n)
    {
        memcpy(IOTJS_REFERENCE_POINTER(str), s, n);
    }
    return str;
}

string_t strings_increment(const string_t *s)
{
    if (!s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, result);
        return result;
    }
    s->reference->count++;
    return *s;
}
BOOL strings_decrement(string_t *s)
{
    if (!s->reference)
    {
        return FALSE;
    }
    BOOL deleted = FALSE;
    switch (s->reference->count)
    {
    case 0:
        break;
    case 1:
        s->reference->count = 0;
        free(s->reference);
        deleted = TRUE;
        break;
    default:
        s->reference->count--;
        break;
    }
    memset(s, 0, sizeof(string_t));
    return deleted;
}
string_t strings_slice(const string_t *s, size_t start)
{
    if (!s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, s0);
        return s0;
    }
    size_t end = s->len;
    return strings_slice2(s, start, end);
}
string_t strings_slice2(const string_t *s, size_t start, size_t end)
{
    if (!s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, s0);
        return s0;
    }
    if (end < start)
    {
        end = start;
    }
    size_t len = end - start;
    start += s->offset;
    if (start >= s->reference->cap)
    {
        IOTJS_VAR_STRUCT(string_t, s0);
        return s0;
    }

    string_t s0 = {
        .offset = start,
        .len = len,
        .reference = s->reference,
    };
    s->reference->count++;
    return s0;
}
size_t strings_copy(const string_t *dst, const string_t *src)
{
    size_t n = dst->len < src->len ? dst->len : src->len;
    if (n > 0)
    {
        memmove(dst->reference->ptr + dst->offset, src->reference->ptr + src->offset, n);
    }
    return n;
}
string_t strings_append_str(const string_t *s, const char *o, size_t n)
{
    if (!s->reference)
    {
        return strings_from_str(o, n);
    }
    size_t cap = s->reference->cap - s->offset;
    size_t len = s->len + n;
    if (len <= cap)
    {
        string_t s0 = {
            .len = len,
            .offset = s->offset,
            .reference = s->reference,
        };
        memmove(s->reference->ptr + s->offset + s->len, o, n);
        s0.reference->count++;
        return s0;
    }
    // malloc
    cap *= 2;
    if (len > cap)
    {
        cap = len;
    }

    string_t s0 = strings_make_cap(len, cap);
    if (s0.reference)
    {
        memcpy(s0.reference->ptr, s->reference->ptr + s->offset, s->len);
        memmove(s->reference->ptr + s->offset + s->len, o, n);
    }
    return s0;
}

string_t strings_append_c_str(const string_t *s, const char *o)
{
    return o ? strings_append_str(s, o, strlen(o)) : strings_increment(s);
}

string_t strings_append(const string_t *s, const string_t *o)
{
    return o && o->reference ? strings_append_str(s, o->reference->ptr + o->offset, o->len) : strings_increment(s);
}