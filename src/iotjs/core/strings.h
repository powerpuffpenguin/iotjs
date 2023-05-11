#ifndef IOTJS_CORE_STRINGS_H
#define IOTJS_CORE_STRINGS_H
#include <stddef.h>
#include <iotjs/core/defines.h>

// 字符串引用
typedef struct
{
    // 字符串內存容量
    size_t cap;
    // 當前被多少變量引用，爲0表示沒有引用系統應該釋放它，否則它正在被使用不能釋放
    size_t count;
    // 實際的內存
    char *ptr;
} strings_reference_t;
// 字符串用於替代 c 過時且難用的 char*
typedef struct
{
    // 字符串大小寫
    size_t len;
    // 字符串的真實內存地址
    size_t offset;
    // 內存引用，如果爲 NULL 則表示此字符串無效，通常是申請內存失敗所以無法創建
    strings_reference_t *reference;
} string_t;
// 創建長度容量都爲 len 的字符串
string_t strings_make(size_t len);
// 以指定長度和容量創建字符串，如果 cap < len 則自動設置 cap = len
string_t strings_make_cap(size_t len, size_t cap);
// 由 c 字符串創建 string_t
string_t strings_from_c_str(const char *s);
// 由指定長度的 char* 創建 string_t
string_t strings_from_str(const char *s, size_t n);

// 創建一個 s 的副本並將內存引用 reference++
// 如果 s 無效，將返回一個同樣無效的 string_t
string_t strings_increment(const string_t *s);
// 表示變量不再使用字符串，它會將內存引用 reference--,
// 如果此後 reference 便爲 0 將會自動釋放內存
BOOL strings_decrement(string_t *s);

// 取一個字符串的切片s[start:]
// 如果 s/start 無效，返回一個無效的字符串
// 如果 delete_s 爲 true，自動爲 s 調用 strings_decrement
string_t strings_slice(string_t *s, size_t start, BOOL delete_s);
// 取一個字符串的切片s[start:end>=start?end:start]
// 如果 s/start/end 無效，返回一個無效的字符串
// 如果 delete_s 爲 true，自動爲 s 調用 strings_decrement
string_t strings_slice_end(string_t *s, size_t start, size_t end, BOOL delete_s);
// 複製字符串並返回實際複製的數據長度
size_t strings_copy(const string_t *dst, const string_t *src);
// 複製字符串並返回實際複製的數據長度
size_t strings_copy_str(const string_t *dst, const char *src, size_t n);
// 複製字符串並返回實際複製的數據長度
size_t strings_copy_c_str(const string_t *dst, const char *src);

// 在字符串末尾添加字符並返回添加後的切
// 如果 delete_s 爲 true，自動爲 s 調用 strings_decrement
string_t strings_append_str(string_t *s, const char *o, size_t n, BOOL delete_s);
// 在字符串末尾添加字符並返回添加後的切片
// 如果 delete_s 爲 true，自動爲 s 調用 strings_decrement
string_t strings_append_c_str(string_t *s, const char *o, BOOL delete_s);
// 在字符串末尾添加字符並返回添加後的切片
// 如果 delete_s 爲 true，自動爲 s 調用 strings_decrement
// 如果 delete_o 爲 true，自動爲 o 調用 strings_decrement
string_t strings_append(string_t *s, string_t *o, BOOL delete_s, BOOL delete_o);
// 返回一個字符串常量 ""，它永遠不會被釋放
string_t strings_const_empty();

#define IOTJS_STRINGS_SET_CHAR(s, i, c) IOTJS_REFERENCE_PTR(s)[i] = c
#define IOTJS_STRINGS_GET_CHAR(s, i) IOTJS_REFERENCE_PTR(s)[i]
#define IOTJS_STRINGS_PTR_SET_CHAR(s, i, c) IOTJS_REFERENCE_PTR_PTR(s)[i] = c
#define IOTJS_STRINGS_PTR_GET_CHAR(s, i) IOTJS_REFERENCE_PTR_PTR(s)[i]
#endif