#include "assert.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
typedef struct
{
    assert_test_function_t test;
    const char *name;
} _assert_test_t;
typedef struct
{
    const char *name;

    _assert_test_t *tests;
    size_t len;
    size_t cap;
} assert_module_t;
typedef struct
{
    size_t len;
    size_t cap;
    assert_module_t **module;
} assert_modules_t;

assert_modules_t assert_modules = {0, 0, 0};
void assert_append_module(assert_module_t *module)
{
    if (assert_modules.len + 1 > assert_modules.cap)
    {
        size_t cap = assert_modules.cap;
        if (cap == 0)
        {
            cap = 16;
        }
        else if (cap < 128)
        {
            cap *= 2;
        }
        else
        {
            cap += 16;
        }
        assert_module_t **modules = (assert_module_t **)malloc(sizeof(assert_module_t *) * cap);
        if (assert_modules.len)
        {
            memcpy(modules, assert_modules.module, assert_modules.len);
            free(assert_modules.module);
        }
        assert_modules.module = modules;
        assert_modules.cap = cap;
    }
    assert_modules.module[assert_modules.len] = module;
    assert_modules.len++;
}
assert_module_t *assert_get_module(const char *module)
{
    assert_module_t **modules = assert_modules.module;
    for (size_t i = 0; i < assert_modules.len; i++)
    {
        if (!strcmp(modules[i]->name, module))
        {
            return modules[i];
        }
    }
    assert_module_t *m = (assert_module_t *)malloc(sizeof(assert_module_t));
    m->name = module;
    m->len = 0;
    m->cap = 0;
    assert_append_module(m);
    return m;
}
void assert_test_register(const char *module, const char *name, assert_test_function_t f)
{
    assert_module_t *m = assert_get_module(module);
    if (m->len + 1 > m->cap)
    {
        size_t cap = m->cap;
        if (cap == 0)
        {
            cap = 16;
        }
        else if (cap < 128)
        {
            cap *= 2;
        }
        else
        {
            cap += 32;
        }
        _assert_test_t *tests = (_assert_test_t *)malloc(sizeof(_assert_test_t) * cap);
        if (m->len)
        {
            memcpy(tests, m->tests, m->len);
            free(m->tests);
        }
        m->tests = tests;
        m->cap = cap;
    }
    _assert_test_t test = {f, name};
    m->tests[m->len] = test;
    m->len++;
}

int assert_run_test(const char *module, const char *name, assert_test_function_t f)
{
    clock_t start = clock();
    assert_test_t test = {module, name};
    int ok = f(&test);
    clock_t finish = clock();
    double used = (double)(finish - start) / CLOCKS_PER_SEC;
    if (ok)
    {
        printf(" - \e[95mfailed\e[0m %s %fs\n", name, used);
    }
    else
    {
        printf(" - \e[92mpassed\e[0m %s %fs\n", name, used);
    }

    return ok;
}
int assert_run_tests(int argc, char *argv[])
{
    size_t total_passed = 0;
    size_t total_failed = 0;
    clock_t start = clock();
    assert_module_t **modules = assert_modules.module;
    for (size_t i = 0; i < assert_modules.len; i++)
    {
        assert_module_t *module = modules[i];
        size_t passed = 0;
        size_t failed = 0;
        printf("test module: %s\n", module->name);
        clock_t start = clock();
        for (size_t j = 0; j < module->len; j++)
        {
            _assert_test_t *test = module->tests + j;
            if (assert_run_test(module->name, test->name, test->test))
            {
                total_failed++;
                failed++;
            }
            else
            {
                total_passed++;
                passed++;
            }
        }
        clock_t finish = clock();
        double used = (double)(finish - start) / CLOCKS_PER_SEC;
        if (failed > 0)
        {
            printf(" * %zu passed, \e[95m%zu failed\e[0m, used %fs\n", passed, failed, used);
        }
        else
        {
            printf(" * %zu passed, %zu failed, used %fs\n", passed, failed, used);
        }
    }

    clock_t finish = clock();
    double used = (double)(finish - start) / CLOCKS_PER_SEC;
    if (total_failed > 0)
    {
        printf("test %zu modules, %zu passed, \e[95m%zu failed\e[0m, used %fs\n", assert_modules.len, total_passed, total_failed, used);
    }
    else
    {
        printf("test %zu modules, %zu passed, %zu failed, used %fs\n", assert_modules.len, total_passed, total_failed, used);
    }

    return 0;
}
int assert_equal(assert_test_t *test,
                 const char *file, int line, const char *func,
                 int type, uintptr_t expected, uintptr_t actual, size_t n,
                 const char *error, const char *message)
{
    int unknow = 0;
    switch (type)
    {
    case ASSERT_TYPE_SIZE_T:
    case ASSERT_TYPE_INT:
    case ASSERT_TYPE_PTR:
        if (expected == actual)
        {
            return 0;
        }
        break;
    case ASSERT_TYPE_C_STR:
        if (!strcmp((char *)expected, (char *)actual))
        {
            return 0;
        }
        break;
    case ASSERT_TYPE_STR:
        if (!n || !memcmp((void *)expected, (void *)actual, n))
        {
            return 0;
        }
        break;
    default:
        unknow = 1;
        break;
    }

    int i = strlen(file) - 1;
    const char *filename = file;
    for (; i >= 0; i--)
    {
        if (file[i] == '/' || file[i] == '\\')
        {
            filename = file + i + 1;
            break;
        }
    }
    if (unknow)
    {
        printf("--- FAIL: module=%s test=%s\n    %s:%d\n      %12s: %s:%d\n      %12s: %d\n", test->module, test->name,
               filename, line,
               "Error Trace", file, line,
               "Error Type", type);
    }
    else
    {
        printf("--- FAIL: module=%s test=%s\n    %s:%d\n      %12s: %s:%d\n      %12s: %s\n", test->module, test->name,
               filename, line,
               "Error Trace", file, line,
               "Error", error);
    }

    switch (type)
    {
    case ASSERT_TYPE_SIZE_T:
        printf("      %12s: %zu\n", "expected", (size_t)expected);
        printf("      %12s: %zu\n", "actual", (size_t)actual);
        break;
    case ASSERT_TYPE_INT:
        printf("      %12s: %d\n", "expected", (int)expected);
        printf("      %12s: %d\n", "actual", (int)actual);
        break;
    case ASSERT_TYPE_PTR:
        printf("      %12s: %lu\n", "expected", expected);
        printf("      %12s: %lu\n", "actual", actual);
        break;
    case ASSERT_TYPE_C_STR:
        printf("      %12s: %s\n", "expected", (char *)expected);
        printf("      %12s: %s\n", "actual", (char *)actual);
        break;
    case ASSERT_TYPE_STR:
    {
        char *s = (char *)malloc(n + 1);
        s[n] = 0;
        memcpy(s, (void *)expected, n);
        printf("      %12s: %s\n", "expected", s);
        memcpy(s, (void *)actual, n);
        printf("      %12s: %s\n", "actual", s);
        free(s);
    }
    break;
    }
    if (message)
    {
        printf("      %12s: %s\n", "Message", message);
    }
    printf("      %12s: %s\n", "Test", func);
    return 1;
}