#include <iotjs/core/number.h>
#include <iotjs/core/defines.h>
void vm_push_size(duk_context *ctx, size_t val)
{
    if (val > IOTJS_MAX_SAFE_INTEGER)
    {
        char s[21] = "";
        snprintf(s, 21, "%zu", val);
        duk_push_string(ctx, s);
    }
    else
    {
        duk_push_number(ctx, (duk_double_t)val);
    }
}
void vm_push_uint64(duk_context *ctx, uint64_t val)
{
    if (val > (uint64_t)(IOTJS_MAX_SAFE_INTEGER))
    {
        char s[21] = "";
        snprintf(s, 21, "%" PRId64 "", val);
        duk_push_string(ctx, s);
    }
    else
    {
        duk_push_number(ctx, (duk_double_t)val);
    }
}
void vm_push_int64(duk_context *ctx, int64_t val)
{
    if (val > IOTJS_MAX_SAFE_INTEGER || val < IOTJS_MIN_SAFE_INTEGER)
    {
        char s[21] = "";
        snprintf(s, 21, "%" PRId64 "", val);
        duk_push_string(ctx, s);
    }
    else
    {
        duk_push_number(ctx, (duk_double_t)val);
    }
}