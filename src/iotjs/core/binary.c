#include <iotjs/core/binary.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
byte_order_t iotjs_little_endian;
byte_order_t iotjs_big_endian;

static uint16_t big_uint16(uint8_t *b)
{
    return (uint16_t)(b[1]) |
           ((uint16_t)(b[0]) << 8);
}
static uint16_t little_uint16(uint8_t *b)
{
    return ((uint16_t)(b[1]) << 8) |
           (uint16_t)(b[0]);
}
static void big_put_uint16(uint8_t *b, uint16_t v)
{
    b[1] = (uint8_t)(v);
    b[0] = (uint8_t)(v >> 8);
}
static void little_put_uint16(uint8_t *b, uint16_t v)
{
    b[1] = (uint8_t)(v >> 8);
    b[0] = (uint8_t)(v);
}
static uint32_t big_uint32(uint8_t *b)
{
    return (uint32_t)(b[3]) |
           ((uint32_t)(b[2]) << 8) |
           ((uint32_t)(b[1]) << 16) |
           ((uint32_t)(b[0]) << 24);
}
static uint32_t little_uint32(uint8_t *b)
{
    return ((uint32_t)(b[3]) << 24) |
           ((uint32_t)(b[2]) << 16) |
           ((uint32_t)(b[1]) << 8) |
           (uint32_t)(b[0]);
}
static void big_put_uint32(uint8_t *b, uint32_t v)
{
    b[3] = (uint8_t)(v);
    b[2] = (uint8_t)(v >> 8);
    b[1] = (uint8_t)(v >> 16);
    b[0] = (uint8_t)(v >> 24);
}
static void little_put_uint32(uint8_t *b, uint32_t v)
{
    b[3] = (uint8_t)(v >> 24);
    b[2] = (uint8_t)(v >> 16);
    b[1] = (uint8_t)(v >> 8);
    b[0] = (uint8_t)(v);
}
static uint64_t big_uint64(uint8_t *b)
{
    return (uint64_t)(b[7]) |
           ((uint64_t)(b[6]) << 8) |
           ((uint64_t)(b[5]) << 16) |
           ((uint64_t)(b[4]) << 24) |
           ((uint64_t)(b[3]) << 32) |
           ((uint64_t)(b[2]) << 40) |
           ((uint64_t)(b[1]) << 48) |
           ((uint64_t)(b[0]) << 56);
}
static uint64_t little_uint64(uint8_t *b)
{
    return ((uint64_t)(b[7]) << 56) |
           ((uint64_t)(b[6]) << 48) |
           ((uint64_t)(b[5]) << 40) |
           ((uint64_t)(b[4]) << 32) |
           ((uint64_t)(b[3]) << 24) |
           ((uint64_t)(b[2]) << 16) |
           ((uint64_t)(b[1]) << 8) |
           (uint64_t)(b[0]);
}
void big_put_uint64(uint8_t *b, uint64_t v)
{
    b[7] = (uint8_t)(v);
    b[6] = (uint8_t)(v >> 8);
    b[5] = (uint8_t)(v >> 16);
    b[4] = (uint8_t)(v >> 24);
    b[3] = (uint8_t)(v >> 32);
    b[2] = (uint8_t)(v >> 40);
    b[1] = (uint8_t)(v >> 48);
    b[0] = (uint8_t)(v >> 56);
}
void little_put_uint64(uint8_t *b, uint64_t v)
{
    b[7] = (uint8_t)(v >> 56);
    b[6] = (uint8_t)(v >> 48);
    b[5] = (uint8_t)(v >> 40);
    b[4] = (uint8_t)(v >> 32);
    b[3] = (uint8_t)(v >> 24);
    b[2] = (uint8_t)(v >> 16);
    b[1] = (uint8_t)(v >> 8);
    b[0] = (uint8_t)(v);
}
const char *encodeStd = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char *encodeURL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
char decodeStd[256];
char decodeURL[256];
void __iotjs_binary_init()
{
    byte_order_t *little, *big;

    uint16_t hport = 0x8000;
    uint16_t nport = htons(hport);
    if (hport == nport)
    {
        // system is big endian
        big = &iotjs_little_endian;
        little = &iotjs_big_endian;
    }
    else
    {
        // system is little endian
        little = &iotjs_little_endian;
        big = &iotjs_big_endian;
    }

    big->uint16 = big_uint16;
    big->put_uint16 = big_put_uint16;
    big->uint32 = big_uint32;
    big->put_uint32 = big_put_uint32;
    big->uint64 = big_uint64;
    big->put_uint64 = big_put_uint64;

    little->uint16 = little_uint16;
    little->put_uint16 = little_put_uint16;
    little->uint32 = little_uint32;
    little->put_uint32 = little_put_uint32;
    little->uint64 = little_uint64;
    little->put_uint64 = little_put_uint64;

    // base64
    memset(decodeStd, 0xff, 256);
    memset(decodeURL, 0xff, 256);
    for (uint8_t i = 0; i < 64; i++)
    {
        decodeStd[encodeStd[i]] = i;
        decodeURL[encodeURL[i]] = i;
    }
}

static unsigned int base64_encoded_len(unsigned int n)
{
    return (n + 2) / 3 * 4; // minimum # 4-char quanta, 3 bytes each
}
static unsigned int base64_encoded_len_no_padding(unsigned int n)
{
    return (n * 8 + 5) / 6; // minimum # chars at 6 bits per char
}
static unsigned int base64_decoded_len(unsigned int n)
{
    return n / 4 * 3; // Padded base64 should always be a multiple of 4 characters in length.
}
static unsigned int base64_decoded_len_no_padding(unsigned int n)
{
    return n % 4 == 1 ? 0 : n * 6 / 8; // Unpadded data may end with partial block of 2-3 characters.
}

unsigned int iotjs_base64_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *encode)
{
    unsigned int ret = pading ? ((src_len + 2) / 3 * 4) : (src_len * 8 + 5) / 6;
    if (ret && dst)
    {
        unsigned int di = 0, si = 0;
        unsigned int n = (src_len / 3) * 3;
        uint32_t val;
        while (src_len > 3)
        {
            dst[0] = encode[src[0] >> 2];
            dst[1] = encode[((src[0] & 0x3) << 4) | (src[1] >> 4)];
            dst[2] = encode[((src[1] & 0xF) << 2) | (src[2] >> 6)];
            dst[3] = encode[src[2] & 0x3F];

            src_len -= 3;
            src += 3;
            dst += 4;
        }
        switch (src_len)
        {
        case 1:
            dst[0] = encode[src[0] >> 2];
            dst[1] = encode[((src[0] & 0x3) << 4)];
            if (pading)
            {
                dst[2] = pading;
                dst[3] = pading;
            }
            break;
        case 2:
            dst[0] = encode[src[0] >> 2];
            dst[1] = encode[((src[0] & 0x3) << 4) | (src[1] >> 4)];
            dst[2] = encode[((src[1] & 0xF) << 2)];
            if (pading)
            {
                dst[3] = pading;
            }
            break;
        }
    }
    return ret;
}
static unsigned int base64_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return iotjs_base64_encode(dst, src, src_len, '=', encodeStd);
}
static unsigned int base64_encode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return iotjs_base64_encode(dst, src, src_len, 0, encodeStd);
}
static unsigned int base64_url_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return iotjs_base64_encode(dst, src, src_len, '=', encodeURL);
}
static unsigned int base64_url_encode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return iotjs_base64_encode(dst, src, src_len, 0, encodeURL);
}
unsigned int iotjs_base64_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *decode)
{
    if (!src_len)
    {
        return 0;
    }
    if (pading)
    {
        if ((src_len % 4))
        {
            return 0; // 填充時 src_len 顯然必須4的整數倍才合法
        }
    }
    else
    {
        if (src_len % 4 == 1)
        {
            return 0; // 非填充時，只會剩餘 2 or 3 個字節，剩餘 1 字節顯然不是 合法 編碼值
        }
    }
    int ret = pading ? (src_len / 4 * 3) : (src_len * 6 / 8);
    if (ret && dst)
    {
        uint8_t s[4];
        uint8_t j;
        // 執行 n-1 次解碼
        unsigned int count = (pading ? src_len : src_len + 3) / 4;
        for (unsigned int i = 1; i < count; i++)
        {
            for (j = 0; j < 4; j++)
            {
                s[j] = j < decode[src[j]];
                if (s[j] == 0xff)
                {
                    // 中途遇到填充 顯然不合法
                    return 0;
                }
            }
            dst[0] = (s[0] << 2) | (s[1] >> 4);
            dst[1] = (s[1] << 4) | (s[2] >> 2);
            dst[2] = (s[2] << 6) | (s[3]);

            dst += 3;
            src += 4;
            src_len -= 4;
        }
        // 執行最後一次解碼，要考慮 填充字符或非填充情況下 src 長度不足 4
        if (pading)
        {
            for (j = 0; j < 4; j++)
            {
                s[j] = decode[src[j]];
                if (s[j] == 0xff)
                {
                    if (src[j] != pading || j < 2) // 不是填充符或在最後兩字節填充
                    {
                        return 0;
                    }
                }
            }
            if (s[2] == 0xff) // 填充了兩個字符
            {
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                if (s[3] != 0xff) // 必須連續兩個相同填充
                {
                    return 0;
                }
            }
            else if (s[3] = 0xff) // 填充了一個字符
            {
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                dst[1] = (s[1] << 4) | (s[2] >> 2);
            }
            else // 沒有填充字符
            {
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                dst[1] = (s[1] << 4) | (s[2] >> 2);
                dst[2] = (s[2] << 6) | (s[3]);
            }
        }
        else
        {
            for (j = 0; j < src_len; j++)
            {
                s[j] = decode[src[j]];
                if (s[j] == 0xff)
                {
                    return 0;
                }
            }
            switch (src_len)
            {
            case 2: // 需要填充兩個字符
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                break;
            case 3: // 需要填充一個字符
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                dst[1] = (s[1] << 4) | (s[2] >> 2);
                break;
            case 4: // 不需要填充
                dst[0] = (s[0] << 2) | (s[1] >> 4);
                dst[1] = (s[1] << 4) | (s[2] >> 2);
                dst[2] = (s[2] << 6) | (s[3]);
                break;
            }
        }
    }
    return ret;
}
static unsigned int base64_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? iotjs_base64_decode(dst, src, src_len, '=', decodeStd) : 0;
}
static unsigned int base64_decode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? iotjs_base64_decode(dst, src, src_len, 0, decodeStd) : 0;
}
static unsigned int base64_url_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? iotjs_base64_decode(dst, src, src_len, '=', decodeURL) : 0;
}
static unsigned int base64_url_decode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? iotjs_base64_decode(dst, src, src_len, 0, decodeURL) : 0;
}
base64_encodes_t iotjs_base64 = {
    .std = {
        .encode = base64_encode,
        .decode = base64_decode,
        .encoded_len = base64_encoded_len,
        .decoded_len = base64_decoded_len,
    },
    .raw_std = {
        .encode = base64_encode_no_padding,
        .decode = base64_decode_no_padding,
        .encoded_len = base64_encoded_len_no_padding,
        .decoded_len = base64_decoded_len_no_padding,
    },
    .url = {
        .encode = base64_url_encode,
        .decode = base64_url_decode,
        .encoded_len = base64_encoded_len,
        .decoded_len = base64_decoded_len,
    },
    .raw_url = {
        .encode = base64_url_encode_no_padding,
        .decode = base64_url_decode_no_padding,
        .encoded_len = base64_encoded_len_no_padding,
        .decoded_len = base64_decoded_len_no_padding,
    },
};
