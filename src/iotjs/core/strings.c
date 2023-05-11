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
    if (!s || !s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, result);
        return result;
    }
    s->reference->count++;
    return *s;
}
BOOL strings_decrement(string_t *s)
{
    if (!s || !s->reference)
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
string_t strings_slice(string_t *s, size_t start, BOOL delete_s)
{
    if (!s || !s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, s0);
        return s0;
    }
    size_t end = s->len;
    return strings_slice_end(s, start, end, delete_s);
}
string_t strings_slice_end(string_t *s, size_t start, size_t end, BOOL delete_s)
{
    if (!s || !s->reference)
    {
        IOTJS_VAR_STRUCT(string_t, s0);
        return s0;
    }
    if (end < start)
    {
        end = start;
    }
    string_t result;
    size_t len = end - start;
    start += s->offset;
    if (start >= s->reference->cap)
    {
        memset(&result, 0, sizeof(string_t));
        if (delete_s)
        {
            strings_decrement(s);
        }
        return result;
    }
    size_t max = s->reference->cap - start;
    result.offset = start;
    result.len = len > max ? max : len;
    result.reference = s->reference;

    s->reference->count++;
    if (delete_s)
    {
        strings_decrement(s);
    }
    return result;
}
size_t strings_copy(const string_t *dst, const string_t *src)
{
    if (!dst || !src || !dst->reference || !src->reference)
    {
        return 0;
    }
    size_t n = dst->len < src->len ? dst->len : src->len;
    if (n > 0)
    {
        memmove(dst->reference->ptr + dst->offset, src->reference->ptr + src->offset, n);
    }
    return n;
}
string_t strings_append_str(string_t *s, const char *o, size_t n, BOOL delete_s)
{
    string_t result;
    if (n == 0)
    {
        if (s && s->reference)
        {
            result = strings_increment(s);
            if (delete_s)
            {
                strings_decrement(s);
            }
        }
        else
        {
            memset(&result, 0, sizeof(string_t));
        }
        return result;
    }
    size_t cap = 0;
    size_t len = n;
    if (s && s->reference)
    {
        cap = s->reference->cap - s->offset;
        len += s->len;
    }
    if (len <= cap)
    {
        result.len = len;
        result.offset = s->offset;
        result.reference = s->reference;
        if (n)
        {
            memmove(s->reference->ptr + s->offset + s->len, o, n);
        }
        s->reference->count++;
        if (delete_s)
        {
            strings_decrement(s);
        }
        return result;
    }
    // malloc
    if (cap < 32)
    {
        cap = 64;
    }
    else
    {
        cap *= 2;
    }
    if (len > cap)
    {
        cap = len;
    }
    result = strings_make_cap(len, cap);
    if (result.reference)
    {
        if (s && s->reference)
        {
            memcpy(result.reference->ptr, s->reference->ptr + s->offset, s->len);
            memcpy(result.reference->ptr + s->len, o, n);
        }
        else
        {
            memcpy(result.reference->ptr, o, n);
        }
    }
    if (delete_s)
    {
        strings_decrement(s);
    }
    return result;
}

string_t strings_append_c_str(string_t *s, const char *o, BOOL delete_s)
{
    return strings_append_str(s, o, o ? strlen(o) : 0, delete_s);
}

string_t strings_append(string_t *s, string_t *o, BOOL delete_s, BOOL delete_o)
{
    if (!o || !o->reference)
    {
        return strings_append_str(s, NULL, 0, delete_s);
    }
    string_t result = strings_append_str(s, o->reference->ptr + o->offset, o->len, delete_s);
    if (delete_o)
    {
        strings_decrement(o);
    }
    return result;
}