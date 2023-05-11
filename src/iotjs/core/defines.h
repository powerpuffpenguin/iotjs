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

#endif