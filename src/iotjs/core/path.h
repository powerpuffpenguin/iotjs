/**
 * 這段代碼由 golang 標準庫移植而來 https://pkg.go.dev/path
 *
 * 它只是從純語法上解析以  / 分隔的路徑字符串，大部分 api 和 golang 保存一致，但是
 * c 無法自動管理內存，當首要申請內存且申請失敗時，會返回一個無效的 string_t 實例
 */
#ifndef IOTJS_CORE_PATH_H
#define IOTJS_CORE_PATH_H
#include <iotjs/core/strings.h>
// 返回 path 的最後一個元素
// 在提取最后一个元素之前删除尾部 /
// 如果 path 爲空或無效的 string_t 則返回 .
// 如果 path 完全由 / 組成則返回 /
string_t path_base(string_t *path, BOOL delete_path);
// 返回與 path 等效的最短路徑
// 1. 用一個 / 替代多個連續的 //
// 2. 消除每一個 . (表示當前路徑)
// 3. 消除每個內部的 .. (表示父路徑)
// 4. 消除 .. 開始的 根路徑，將 "/.." 替換爲 "/"
string_t path_clean(string_t *path, BOOL delete_path);

// 返回擴展名，如果沒有則返回一個 空的字符串
string_t path_ext(string_t *path, BOOL delete_path);
BOOL path_is_abs_str(const char *path, size_t n);
BOOL path_is_abs_c_str(const char *path);
BOOL path_is_abs(string_t *path, BOOL delete_path);
typedef struct
{
    string_t dir;
    string_t file;
} path_split_t;
// 在最後一個 / 之後立即分割 path，
// 將其分成 dir 和 file 部分。
// 如果 path 中沒有 /，string_split 返回一個空字符串到 dir 並設置 file=path。
// 返回值具有 path = dir+file 的屬性。
path_split_t path_split(string_t *path, BOOL delete_path);
// 返迴 path 的最後一個元素以外的所有元素，通常是 path 的目錄。
// 在使用 path_split 刪除最後一個元素後，path 被 path_clean 並刪除尾部的 /。
// 如果 path 為空返回 '.'。
// 如果 path 完全由 / 和 非斜線字節組成，則返回一個/。
// 在任何其他情況下，返回的路徑不會以 / 結尾
string_t path_dir(string_t *path, BOOL delete_path);

#endif