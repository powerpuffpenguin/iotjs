#include <iotjs/core/path.h>
#include <string.h>
#include <stdio.h>
typedef struct
{
    string_t *s;
    string_t buf;
    int w;
} _path_lazybuf_t;
#define _PATH_LAZYBUF_APPEND(c)         \
    if (!_path_lazybuf_append(&out, c)) \
    {                                   \
        if (delete_path)                \
        {                               \
            strings_decrement(path);    \
        }                               \
        IOTJS_VAR_STRUCT(string_t, s);  \
        return s;                       \
    }

BOOL _path_lazybuf_append(_path_lazybuf_t *b, const char c)
{

    if (IOTJS_REFERENCE_INVALID(b->buf))
    {
        if (b->w < b->s->len && IOTJS_STRINGS_PTR_GET_CHAR(b->s, b->w) == c)
        {
            b->w++;
            return TRUE;
        }
        b->buf = strings_make(b->s->len);
        if (!b->buf.reference)
        {
            return FALSE;
        }
        strings_copy_str(&b->buf, IOTJS_REFERENCE_PTR_PTR(b->s), b->w);
    }
    IOTJS_STRINGS_SET_CHAR(b->buf, b->w, c);
    b->w++;
    return TRUE;
}

int _path_last_slash(const char *s, int len)
{
    int i = len - 1;
    while (i >= 0 && s[i] != '/')
    {
        i--;
    }
    return i;
}

string_t path_base(string_t *p, BOOL delete_path)
{
    if (IOTJS_REFERENCE_PTR_INVALID(p))
    {
        return strings_from_str(".", 1);
    }
    if (p->len == 0)
    {
        if (delete_path)
        {
            strings_decrement(p);
        }
        return strings_from_str(".", 1);
    }

    string_t path = strings_increment(p);
    if (delete_path)
    {
        strings_decrement(p);
    }
    // Strip trailing slashes.
    while (path.len > 0 && IOTJS_STRINGS_GET_CHAR(path, path.len - 1) == '/')
    {
        path.len--;
    }
    // Find the last element
    int i = _path_last_slash(IOTJS_REFERENCE_PTR(path), path.len);
    if (i >= 0)
    {
        i++;
        path.offset += i;
        path.len -= i;
    }
    // If empty now, it had only slashes.
    if (path.len == 0)
    {
        strings_decrement(&path);
        return strings_from_str("/", 1);
    }
    return path;
}
string_t path_clean(string_t *path, BOOL delete_path)
{
    if (IOTJS_REFERENCE_PTR_INVALID(path))
    {
        return strings_from_str(".", 1);
    }
    BOOL rooted = IOTJS_STRINGS_PTR_GET_CHAR(path, 0) == '/' ? TRUE : FALSE;
    int n = path->len;
    // Invariants:
    //	reading from path; r is index of next byte to process.
    //	writing to buf; w is index of next byte to write.
    //	dotdot is index in buf where .. must stop, either because
    //		it is the leading slash or it is a leading ../../.. prefix.
    _path_lazybuf_t out = {
        .s = path,
        .buf = {
            .len = 0,
            .offset = 0,
            .reference = NULL,
        },
        .w = 0,
    };
    int r = 0, dotdot = 0;
    if (rooted)
    {
        _PATH_LAZYBUF_APPEND('/')
        r = 1;
        dotdot = 1;
    }

    const char *p = IOTJS_REFERENCE_PTR_PTR(path);
    while (r < n)
    {
        if (p[r] == '/')
        {
            // empty path element
            r++;
        }
        else if (p[r] == '.' && (r + 1 == n || p[r + 1] == '/'))
        {
            // . element
            r++;
        }
        else if (p[r] == '.' && p[r + 1] == '.' && (r + 2 == n || p[r + 2] == '/'))
        {
            // .. element: remove to last /
            r += 2;
            if (out.w > dotdot)
            {
                // can backtrack
                out.w--;
                while (out.w > dotdot &&
                       '/' != (IOTJS_REFERENCE_VALID(out.buf) ? IOTJS_STRINGS_GET_CHAR(out.buf, out.w) : IOTJS_STRINGS_PTR_GET_CHAR(out.s, out.w)))
                {
                    out.w--;
                }
            }
            else if (!rooted)
            {
                // cannot backtrack, but not rooted, so append .. element.
                if (out.w > 0)
                {
                    _PATH_LAZYBUF_APPEND('/')
                }
                _PATH_LAZYBUF_APPEND('.')
                _PATH_LAZYBUF_APPEND('.')
                dotdot = out.w;
            }
        }
        else
        {
            // real path element.
            // add slash if needed
            if ((rooted && out.w != 1) || (!rooted && out.w != 0))
            {
                _PATH_LAZYBUF_APPEND('/')
            }
            // copy element
            for (; r < n && p[r] != '/'; r++)
            {
                _PATH_LAZYBUF_APPEND(p[r])
            }
        }
    }

    // Turn empty string into "."
    string_t s;
    if (out.w == 0)
    {
        s = strings_from_str(".", 1);
    }
    else
    {
        if (out.buf.reference)
        {
            s = out.buf;
            s.len = out.w;
        }
        else
        {
            s = strings_slice_end(path, 0, out.w, FALSE);
        }
    }
    if (delete_path)
    {
        strings_decrement(path);
    }
    return s;
}
string_t path_ext(string_t *path, BOOL delete_path)
{
    if (IOTJS_REFERENCE_PTR_INVALID(path))
    {
        return strings_const_empty();
    }

    int i = path->len;
    i--;
    char *s = path->reference->ptr + path->offset;
    for (; i >= 0; i--)
    {
        if (s[i] == '/')
        {
            break;
        }
        if (s[i] == '.')
        {
            return strings_slice(path, i, delete_path);
        }
    }
    if (delete_path)
    {
        strings_decrement(path);
    }
    return strings_const_empty();
}
BOOL path_is_abs_str(const char *path, size_t n)
{
    return (n > 0 && path[0] == '/') ? TRUE : FALSE;
}
BOOL path_is_abs_c_str(const char *path)
{
    return path[0] == '/' ? TRUE : FALSE;
}
BOOL path_is_abs(string_t *path, BOOL delete_path)
{
    if (IOTJS_REFERENCE_PTR_INVALID(path))
    {
        return FALSE;
    }
    BOOL ok = (path->len > 0 && IOTJS_STRINGS_PTR_GET_CHAR(path, 0) == '/') ? TRUE : FALSE;
    if (delete_path)
    {
        strings_decrement(path);
    }
    return ok;
}
path_split_t path_split(string_t *path, BOOL delete_path)
{
    path_split_t result;
    if (IOTJS_REFERENCE_PTR_INVALID(path))
    {
        IOTJS_VAR_STRUCT(string_t, s);
        result.dir = s;
        result.file = s;
        return result;
    }
    int i = _path_last_slash(IOTJS_REFERENCE_PTR_PTR(path), path->len);
    result.dir = strings_slice_end(path, 0, i + 1, FALSE);
    result.file = strings_slice(path, i + 1, FALSE);
    if (delete_path)
    {
        strings_decrement(path);
    }
    return result;
}
string_t path_dir(string_t *path, BOOL delete_path)
{
    if (IOTJS_REFERENCE_PTR_INVALID(path))
    {
        return strings_from_str(".", 1);
    }
    path_split_t split = path_split(path, delete_path);
    strings_decrement(&split.file);
    return path_clean(&split.dir, TRUE);
}