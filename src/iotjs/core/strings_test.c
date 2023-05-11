#include <iotjs/assert/assert.h>
#include <iotjs/core/strings.h>
int test_strings_make(assert_test_t *test)
{
    string_t s = strings_make(10);
    ASSERT_EQUAL_SIZE_T(test, s.offset, 0)
    ASSERT_EQUAL_SIZE_T(test, s.len, 10)
    ASSERT_EQUAL_SIZE_T(test, s.reference->cap, 10)
    ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s), 1)
    ASSERT_EQUAL_C_STR(test, IOTJS_REFERENCE_PTR(s), "")
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));

    s = strings_make_cap(10, 15);
    ASSERT_EQUAL_SIZE_T(test, s.offset, 0)
    ASSERT_EQUAL_SIZE_T(test, s.len, 10)
    ASSERT_EQUAL_SIZE_T(test, s.reference->cap, 15)
    ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s), 1)
    ASSERT_EQUAL_C_STR(test, IOTJS_REFERENCE_PTR(s), "")
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));

    const char *c_str = "cerberus is an idea";
    size_t len = strlen(c_str);
    s = strings_from_c_str(c_str);
    ASSERT_EQUAL_SIZE_T(test, s.offset, 0)
    ASSERT_EQUAL_SIZE_T(test, s.len, len)
    ASSERT_EQUAL_SIZE_T(test, s.reference->cap, len)
    ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s), 1)
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, len)
    {
        string_t s0 = strings_increment(&s);
        ASSERT_EQUAL_SIZE_T(test, s.offset, 0)
        ASSERT_EQUAL_SIZE_T(test, s.len, len)
        ASSERT_EQUAL_SIZE_T(test, s.reference->cap, len)
        ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s), 2)
        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, len)

        ASSERT_EQUAL_SIZE_T(test, s0.offset, 0)
        ASSERT_EQUAL_SIZE_T(test, s0.len, len)
        ASSERT_EQUAL_SIZE_T(test, s0.reference->cap, len)
        ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s0), 2)
        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s0), c_str, len)

        ASSERT_EQUAL_PTR(test, s.reference, s0.reference)
        ASSERT_EQUAL_INT(test, FALSE, strings_decrement(&s0))
    }
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s))

    c_str = "kate is so cute";
    len = strlen(c_str);
    s = strings_from_str(c_str, len);
    ASSERT_EQUAL_SIZE_T(test, s.offset, 0)
    ASSERT_EQUAL_SIZE_T(test, s.len, len)
    ASSERT_EQUAL_SIZE_T(test, s.reference->cap, len)
    ASSERT_EQUAL_SIZE_T(test, IOTJS_REFERENCE_COUNT(s), 1)
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, len)
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));

    return 0;
}
ASSERT_TEST_FUNC(strings, make, test_strings_make)

int test_strings_slice(assert_test_t *test)
{
    const char *c_str = "kate is so cute";
    size_t len = strlen(c_str);
    string_t s = strings_from_str(c_str, len);
    string_t s0 = strings_slice(&s, 5, FALSE);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, len);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s0), c_str + 5, len - 5);
    string_t s1 = strings_slice_end(&s0, 3, 3 + 2, FALSE);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, len);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s0), c_str + 5, len - 5);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s1), "so", 2);
    IOTJS_STRINGS_SET_CHAR(s1, 0, 'S');
    IOTJS_STRINGS_SET_CHAR(s1, 1, 'O');
    string_t s2 = strings_slice_end(&s1, 0, 6, TRUE);

    const char *c_str1 = "kate is SO cute";
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str1, len);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s0), c_str1 + 5, len - 5);
    // ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s1), "SO", 2);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s2), "SO cut", 6);
    // ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s1), "SO cut", 6);

    ASSERT_EQUAL_INT(test, FALSE, strings_decrement(&s2));
    // ASSERT_EQUAL_INT(test, FALSE, strings_decrement(&s1));
    ASSERT_EQUAL_INT(test, FALSE, strings_decrement(&s0));
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));
    return 0;
}
ASSERT_TEST_FUNC(strings, slice, test_strings_slice)

int test_strings_append(assert_test_t *test)
{
    const char *c_str = "123";
    string_t s = strings_append_c_str(NULL, c_str, TRUE);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), c_str, s.len);
    ASSERT_EQUAL_SIZE_T(test, 3, s.len);
    ASSERT_EQUAL_SIZE_T(test, 64, s.reference->cap);
    ASSERT_EQUAL_SIZE_T(test, 1, s.reference->count);
    s = strings_append_c_str(&s, "456", TRUE);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), "123456", s.len);
    ASSERT_EQUAL_SIZE_T(test, 6, s.len);
    ASSERT_EQUAL_SIZE_T(test, 64, s.reference->cap);
    ASSERT_EQUAL_SIZE_T(test, 1, s.reference->count);
    string_t s0 = strings_slice_end(&s, 3, 7, FALSE);
    ASSERT_EQUAL_SIZE_T(test, 2, s.reference->count);
    ASSERT_EQUAL_SIZE_T(test, 4, s0.len);
    ASSERT_EQUAL_SIZE_T(test, 3, s0.offset);
    IOTJS_STRINGS_SET_CHAR(s0, 3, '7');
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s0), "4567", s0.len);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), "1234567", 7);
    s = strings_append(&s0, &s, TRUE, TRUE);
    ASSERT_EQUAL_SIZE_T(test, 1, s.reference->count);
    ASSERT_EQUAL_SIZE_T(test, 4 + 6, s.len);
    ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), "4567123456", s.len);
    ASSERT_EQUAL_SIZE_T(test, 3, s.offset);
    ASSERT_EQUAL_STR(test, "1234567123456", s.reference->ptr, 3 + s.len);
    ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));
    return 0;
}
ASSERT_TEST_FUNC(strings, append, test_strings_append)