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

typedef struct
{
    unsigned int (*encoded_len)(unsigned int n);
    void (*encode)(uint8_t *dst, const uint8_t *src, unsigned int src_len);
    unsigned int (*decoded_len)(unsigned int n);
    int (*decode)(uint8_t *dst, const uint8_t *src, unsigned int src_len);
} iotjs_encode_t;
typedef struct
{
    iotjs_encode_t std;
    // 不會填充 =
    iotjs_encode_t raw_std;

    iotjs_encode_t url;
    // 不會填充 =
    iotjs_encode_t raw_url;
} base64_encodes_t;
extern base64_encodes_t iotjs_base64;
#endif
