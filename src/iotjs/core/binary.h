#ifndef IOTJS_CORE_BINARY_H
#define IOTJS_CORE_BINARY_H
#include <stdint.h>
typedef struct
{
    uint16_t (*uint16)(uint8_t *b);
    uint32_t (*uint32)(uint8_t *b);
    uint64_t (*uint64)(uint8_t *b);
    void (*put_uint16)(uint8_t *b, uint16_t v);
    void (*put_uint32)(uint8_t *b, uint32_t v);
    void (*put_uint64)(uint8_t *b, uint64_t v);
} byte_order_t;
extern byte_order_t iotjs_little_endian;
extern byte_order_t iotjs_big_endian;
#endif
