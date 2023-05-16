#ifndef IOTJS_CORE_JS_THREADS_H
#define IOTJS_CORE_JS_THREADS_H
#include <iotjs/core/js.h>
#include <thpool.h>
typedef struct thpool_ threads_t;
void _vm_init_threads(duk_context *ctx);
// duk_throw
// 成功返回 線程池 ... => ...
threads_t *vm_get_threads(duk_context *ctx);

typedef void (*vm_thread_work_t)(void *);
// 向線程池添加一個工作
BOOL vm_add_work(threads_t *threads, vm_thread_work_t work, void *arg);

#endif