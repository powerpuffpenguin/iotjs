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

// 返回引用類型是否有效
#define IOTJS_REFERENCE_VALID(var) ((var).reference)
// 返回引用計數
#define IOTJS_REFERENCE_COUNT(var) ((var).reference->count)

// 返回引用類型引用對象的內存地址
#define IOTJS_REFERENCE_POINTER(var) ((var).reference->ptr + (var).offset)

// 定義一個 struct 的實例，並將所有屬性設置爲 0
#define IOTJS_VAR_STRUCT(type, var) \
    type var;                       \
    memset(&var, 0, sizeof(type))

#endif