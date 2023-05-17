#ifndef IOTJS_CORE_DEFINES_H
#define IOTJS_CORE_DEFINES_H

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef IOTJS_MALLOC
#define IOTJS_MALLOC(size) malloc(size)
#endif

#ifndef IOTJS_FREE
#define IOTJS_FREE(p) free(p)
#endif

#define VM_STASH_KEY_IOTJS "iotjs", 5
#define VM_STASH_KEY_C "c", 1
#define VM_STASH_KEY_NATIVE "native", 6
#define VM_STASH_KEY_TIMEOUT "timeout", 7
#define VM_STASH_KEY_INTERVAL "interval", 8
#define VM_STASH_KEY_JOBS "jobs", 4

#define VM_IOTJS_KEY_COMPLETER "Completer", 9
#define VM_IOTJS_KEY_ERROR "IotError", 8

// 檢測引用類型 有效
#define IOTJS_REFERENCE_VALID(var) (var.reference)
// 檢測引用類型指針 有效
#define IOTJS_REFERENCE_PTR_VALID(var) (var && var->reference)
// 檢測引用類型 無效
#define IOTJS_REFERENCE_INVALID(var) (!var.reference)
// 檢測引用類型指針 無效
#define IOTJS_REFERENCE_PTR_INVALID(var) (!var || !var->reference)

// 返回引用計數
#define IOTJS_REFERENCE_COUNT(var) (var.reference->count)

// 返回引用類型引用對象 的內存地址
#define IOTJS_REFERENCE_PTR(var) (var.reference->ptr + var.offset)
// 返回引用類型引用指針 的對象內存地址
#define IOTJS_REFERENCE_PTR_PTR(var) (var->reference->ptr + var->offset)

// 定義一個 struct 的實例，並將所有屬性設置爲 0
#define IOTJS_VAR_STRUCT(type, var) \
    type var;                       \
    memset(&var, 0, sizeof(type))

// 2**53-1
#define IOTJS_MAX_SAFE_INTEGER 9007199254740991
// -(2**53-1)
#define IOTJS_MIN_SAFE_INTEGER -9007199254740991

#define IOTJS_MAX_INT64 9223372036854775807
#define IOTJS_MIN_INT64 -9223372036854775808
#define IOTJS_MAX_UINT64 18446744073709551615
#define IOTJS_MAX_INT32 2147483647
#define IOTJS_MIN_INT32 -2147483648
#define IOTJS_MAX_UINT32 4294967295
#define IOTJS_MAX_INT16 32767
#define IOTJS_MIN_INT16 -32768
#define IOTJS_MAX_UINT16 65535
#define IOTJS_MAX_INT8 127
#define IOTJS_MIN_INT8 -128
#define IOTJS_MAX_UINT8 255

#endif