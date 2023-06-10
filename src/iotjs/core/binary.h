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
    // 將 src base64 編碼到 dst，可以爲 dst 傳入 NULL 來僅僅計算 dst 需要準備多少字節以備寫入
    unsigned int (*encode)(uint8_t *dst, const uint8_t *src, unsigned int src_len);
    unsigned int (*decoded_len)(unsigned int n);
    // jiang src base64 解碼到 dst，可以爲 dst 傳入 NULL 來僅僅計算 dst 需要準備多少字節以備寫入，
    // 如果 src 不是合法的編碼值，將返回 0
    unsigned int (*decode)(uint8_t *dst, const uint8_t *src, unsigned int src_len);
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

// 將 src 使用 base64 編碼到 dst，
// pading 是填充字符，如果爲0則不進行填充，
// encode 是 64 字節的編碼值，標準算法時應該是 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
// 成功返回 dst 被寫入字節數，你可以將 dst 設置爲 NULL 來計算 dst 需要被寫入的字節
unsigned int iotjs_base64_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *encode);
// 將 src 使用 base64 解碼到 dst，
// pading 是填充字符，如果爲0則表示 src 中沒有填充，
// decode 是 256 字節的解碼索引，memset(decode, 0xFF, 256); for(int i=0;i<64;i++) decode[encode[i]]=i;
// 成功返回 dst 被寫入字節數，你可以將 dst 設置爲 NULL 來計算 dst 需要被寫入的字節
// 如果 src 不是合法的編碼值，將返回 0
unsigned int iotjs_base64_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *decode);
#endif
