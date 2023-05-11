#include <iotjs/assert/assert.h>
#include <iotjs/core/path.h>
#include <iotjs/core/strings.h>

int test_path_clean(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const char *result;
    } test_t;
    test_t tests[] = {
        // Already clean
        {"", "."},
        {"abc", "abc"},
        {"abc/def", "abc/def"},
        {"a/b/c", "a/b/c"},
        {".", "."},
        {"..", ".."},
        {"../..", "../.."},
        {"../../abc", "../../abc"},
        {"/abc", "/abc"},
        {"/", "/"},

        // Remove trailing slash
        {"abc/", "abc"},
        {"abc/def/", "abc/def"},
        {"a/b/c/", "a/b/c"},
        {"./", "."},
        {"../", ".."},
        {"../../", "../.."},
        {"/abc/", "/abc"},

        // Remove doubled slash
        {"abc//def//ghi", "abc/def/ghi"},
        {"//abc", "/abc"},
        {"///abc", "/abc"},
        {"//abc//", "/abc"},
        {"abc//", "abc"},

        // Remove . elements
        {"abc/./def", "abc/def"},
        {"/./abc/def", "/abc/def"},
        {"abc/.", "abc"},

        // Remove .. elements
        {"abc/def/ghi/../jkl", "abc/def/jkl"},
        {"abc/def/../ghi/../jkl", "abc/jkl"},
        {"abc/def/..", "abc"},
        {"abc/def/../..", "."},
        {"/abc/def/../..", "/"},
        {"abc/def/../../..", ".."},
        {"/abc/def/../../..", "/"},
        {"abc/def/../../../ghi/jkl/../../../mno", "../../mno"},

        // Combinations
        {"abc/./../def", "def"},
        {"abc//./../def", "def"},
        {"abc/../../././../def", "../../def"},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        string_t s = strings_from_c_str(t.path);
        s = path_clean(&s, TRUE);
        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), t.result, s.len);
        ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));
    }
    return 0;
}
ASSERT_TEST_FUNC(path, clean, test_path_clean)

int test_path_split(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const char *dir;
        const char *file;
    } test_t;
    test_t tests[] = {
        {"a/b", "a/", "b"},
        {"a/b/", "a/b/", ""},
        {"a/", "a/", ""},
        {"a", "", "a"},
        {"/", "/", ""},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        string_t path = strings_from_c_str(t.path);
        path_split_t split = path_split(&path, TRUE);

        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(split.dir), t.dir, split.dir.len);
        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(split.file), t.file, split.file.len);

        strings_decrement(&split.dir);
        strings_decrement(&split.file);
    }
    return 0;
}
ASSERT_TEST_FUNC(path, split, test_path_split)

int test_path_ext(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const char *ext;
    } test_t;
    test_t tests[] = {
        {"path.go", ".go"},
        {"path.pb.go", ".go"},
        {"a.dir/b", ""},
        {"a.dir/b.go", ".go"},
        {"a.dir/", ""},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        string_t s = strings_from_c_str(t.path);
        s = path_ext(&s, TRUE);

        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), t.ext, s.len);

        ASSERT_EQUAL_INT(test, s.len != 0, strings_decrement(&s));
    }
    return 0;
}
ASSERT_TEST_FUNC(path, ext, test_path_ext)

int test_path_base(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const char *result;
    } test_t;
    test_t tests[] = {
        // Already clean
        {"", "."},
        {".", "."},
        {"/.", "."},
        {"/", "/"},
        {"////", "/"},
        {"x/", "x"},
        {"abc", "abc"},
        {"abc/def", "def"},
        {"a/b/.x", ".x"},
        {"a/b/c.", "c."},
        {"a/b/c.x", "c.x"},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        string_t s = strings_from_c_str(t.path);
        s = path_base(&s, TRUE);

        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), t.result, s.len);

        ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));
    }
    return 0;
}
ASSERT_TEST_FUNC(path, base, test_path_base)

int test_path_dir(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const char *result;
    } test_t;
    test_t tests[] = {
        {"", "."},
        {".", "."},
        {"/.", "/"},
        {"/", "/"},
        {"////", "/"},
        {"/foo", "/"},
        {"x/", "x"},
        {"abc", "."},
        {"abc/def", "abc"},
        {"abc////def", "abc"},
        {"a/b/.x", "a/b"},
        {"a/b/c.", "a/b"},
        {"a/b/c.x", "a/b"},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        string_t s = strings_from_c_str(t.path);
        s = path_dir(&s, TRUE);

        ASSERT_EQUAL_STR(test, IOTJS_REFERENCE_PTR(s), t.result, s.len);

        ASSERT_EQUAL_INT(test, TRUE, strings_decrement(&s));
    }
    return 0;
}
ASSERT_TEST_FUNC(path, dir, test_path_dir)

int test_path_is_abs(assert_test_t *test)
{
    typedef struct
    {
        const char *path;
        const BOOL isAbs;
    } test_t;
    test_t tests[] = {
        {"", FALSE},
        {"/", TRUE},
        {"/usr/bin/gcc", TRUE},
        {"..", FALSE},
        {"/a/../bb", TRUE},
        {".", FALSE},
        {"./", FALSE},
        {"lala", FALSE},
    };
    size_t n = sizeof(tests) / sizeof(test_t);
    for (size_t i = 0; i < n; i++)
    {
        test_t t = tests[i];
        BOOL ok = path_is_abs_c_str(t.path);
        ASSERT_EQUAL_INT(test, t.isAbs, ok);
        ok = path_is_abs_str(t.path, strlen(t.path));
        ASSERT_EQUAL_INT(test, t.isAbs, ok);
        string_t s = strings_from_c_str(t.path);
        ok = path_is_abs(&s, TRUE);
        ASSERT_EQUAL_INT(test, t.isAbs, ok);

        ASSERT_EQUAL_INT(test, FALSE, strings_decrement(&s));
    }
    return 0;
}
ASSERT_TEST_FUNC(path, is_abs, test_path_is_abs)