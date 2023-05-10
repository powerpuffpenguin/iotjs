#ifndef IOTJS_ASSERT_TEST_H
#define IOTJS_ASSERT_TEST_H
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct
{
    const char *module;
    const char *name;
} assert_test_t;

typedef int (*assert_test_function_t)(assert_test_t *test);

void assert_test_register(const char *module, const char *name, assert_test_function_t f);
int assert_run_tests(int argc, char *argv[]);
int assert_equal(assert_test_t *test,
                 const char *file, int line, const char *func,
                 int type, uintptr_t expected, uintptr_t actual, size_t n,
                 const char *error, const char *message);

#define ASSERT_TYPE_SIZE_T 1
#define ASSERT_TYPE_INT 2
#define ASSERT_TYPE_PTR 3
#define ASSERT_TYPE_C_STR 20
#define ASSERT_TYPE_STR 21

#define ASSERT_TEST(module, name, func)                                    \
    int __iotjs_assert_f_##module##_test_##name(assert_test_t *test) func; \
    __attribute((constructor)) void __iotjs_assert_init_##module##_test_##name() { assert_test_register(#module, #name, __iotjs_assert_f_##module##_test_##name); }

#define ASSERT_TEST_FUNC(module, name, func) \
    __attribute((constructor)) void __iotjs_assert_init_##module##_test_##name() { assert_test_register(#module, #name, func); }

#define __ASSERT_EQUAL_MESSAGE(test, type, expect, actual, n, message)                                                                 \
    if (assert_equal(test, __FILE__, __LINE__, __FUNCTION__, type, (uintptr_t)(expect), (uintptr_t)(actual), n, "not equal", message)) \
    {                                                                                                                                  \
        return 1;                                                                                                                      \
    }

#define ASSERT_EQUAL_SIZE_T_MESSAGE(test, expect, actual, message) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_SIZE_T, expect, actual, 0, message)
#define ASSERT_EQUAL_SIZE_T(test, expect, actual) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_SIZE_T, expect, actual, 0, 0)
#define ASSERT_EQUAL_INT_MESSAGE(test, expect, actual, message) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_INT, expect, actual, 0, message)
#define ASSERT_EQUAL_INT(test, expect, actual) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_INT, expect, actual, 0, 0)
#define ASSERT_EQUAL_PTR_MESSAGE(test, expect, actual, message) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_PTR, expect, actual, 0, message)
#define ASSERT_EQUAL_PTR(test, expect, actual) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_PTR, expect, actual, 0, 0)
#define ASSERT_EQUAL_C_STR_MESSAGE(test, expect, actual, message) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_C_STR, expect, actual, 0, message)
#define ASSERT_EQUAL_C_STR(test, expect, actual) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_C_STR, expect, actual, 0, 0)
#define ASSERT_EQUAL_STR_MESSAGE(test, expect, actual, n, message) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_STR, expect, actual, n, message)
#define ASSERT_EQUAL_STR(test, expect, actual, n) __ASSERT_EQUAL_MESSAGE(test, ASSERT_TYPE_STR, expect, actual, n, 0)

#endif