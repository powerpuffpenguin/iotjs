#ifndef IOTJS_CORE_FINALIZER_H
#define IOTJS_CORE_FINALIZER_H
#include <duktape.h>
// 與終結器關聯的 c 指針
typedef struct
{
    void *p;               // 動態申請的 c 指針地址
    void (*free)(void *p); // c 指針釋放函數
} finalizer_t;

// duk api 出錯時可能永遠不會返回(內部使用 jmp 指令 跳轉到了調用棧外部)，這對於資源釋放是一個挑戰，
// 一個合理的解決方案是，使用 duk 提供的 set_finalizer 將資源釋放代碼和一個 object 關聯，當 object 被釋放時自動調用 資源釋放代碼，
// 然而 set_finalizer 和 創建 關聯 object 也可能出現錯誤。
//
// vm_create_finalizer 創建好一個 object 並將其 finalizer 與 finalizer_t 關聯，如果失敗則 duk_throw，
//
// error ... => ..., e
// success ... => ..., obj
finalizer_t *vm_create_finalizer(duk_context *ctx);
// error ... => ..., e
// success ... => ..., obj
// 類似 vm_create_finalizer, 但自動爲 p 申請內存, p=malloc(n)
finalizer_t *vm_create_finalizer_n(duk_context *ctx, size_t n);
// 如果 終結器 一致 返回 finalizer_t*，否則 duk_throw
finalizer_t *vm_require_finalizer(duk_context *ctx, duk_idx_t idx, void (*freef)(void *p));
// 立刻調用 終結器 釋放資源
finalizer_t *vm_finalizer_free(duk_context *ctx, duk_idx_t idx, void (*freef)(void *p));

#endif