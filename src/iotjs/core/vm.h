#ifndef IOTJS_CORE_VM_H
#define IOTJS_CORE_VM_H
#include <iotjs/core/js.h>
#include <iotjs/core/js_threads.h>

// 加載 最初的 main 腳本
duk_ret_t vm_main(duk_context *ctx, const char *path, int argc, char *argv[]);

// 異步方法
typedef struct vm_async
{
    duk_context *ctx;
    threads_t *threads;
    event_base_t *eb;

    event_t *ev;
    void *in;
    void *out;

    int err;
    const char *msg;

    void (*complete)(struct vm_async *);
} vm_async_t;
// 創建一個異步方法
// ... -> ... keys_finalizer key obj_finalizer
vm_async_t *vm_new_async(duk_context *ctx, size_t in, size_t out);

// #define VM_DEBUG_MODULE_LOAD 1

#endif