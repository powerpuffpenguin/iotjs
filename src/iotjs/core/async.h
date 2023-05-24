#ifndef IOTJS_CORE_ASYNC_H
#define IOTJS_CORE_ASYNC_H
#include <duktape.h>
// 這個一個輔助函數，用於將原本的同步函數 f 用 Promise 包裝爲異步函數，並在工作線程中執行原本的同步代碼
duk_ret_t vm_call_async(duk_context *ctx, duk_c_function f);
#endif