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

static __attribute((constructor)) void __iotjs_byte_order_init()
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
}

const char *encodeStd = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char *encodeURL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
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
    return n * 6 / 8; // Unpadded data may end with partial block of 2-3 characters.
}

static void base64_encode_impl(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *encode)
{
    unsigned int di = 0, si = 0;
    unsigned int n = (src_len / 3) * 3;
    uint32_t val;
    while (si < n)
    {
        // Convert 3x 8bit source bytes into 4 bytes
        val = (uint32_t)(src[si + 0]) << 16 | (uint32_t)(src[si + 1]) << 8 | (uint32_t)(src[si + 2]);

        dst[di + 0] = encode[val >> 18 & 0x3F];
        dst[di + 1] = encode[val >> 12 & 0x3F];
        dst[di + 2] = encode[val >> 6 & 0x3F];
        dst[di + 3] = encode[val & 0x3F];

        si += 3;
        di += 4;
    }

    unsigned int remain = src_len - si;
    if (remain == 0)
    {
        return;
    }
    // Add the remaining small block
    val = (uint32_t)(src[si + 0]) << 16;
    if (remain == 2)
    {
        val |= (uint32_t)(src[si + 1]) << 8;
    }

    dst[di + 0] = encode[val >> 18 & 0x3F];
    dst[di + 1] = encode[val >> 12 & 0x3F];

    switch (remain)
    {
    case 2:
        dst[di + 2] = encode[val >> 6 & 0x3F];
        if (pading)
        {
            dst[di + 3] = pading;
        }
        break;
    case 1:
        if (pading)
        {
            dst[di + 2] = pading;
            dst[di + 3] = pading;
        }
        break;
    }
}
static void base64_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    if (src_len)
    {
        base64_encode_impl(dst, src, src_len, '=', encodeStd);
    }
}
static void base64_encode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    if (src_len)
    {
        base64_encode_impl(dst, src, src_len, 0, encodeStd);
    }
}
static void base64_url_encode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    if (src_len)
    {
        base64_encode_impl(dst, src, src_len, '=', encodeURL);
    }
}
static void base64_url_encode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    if (src_len)
    {
        base64_encode_impl(dst, src, src_len, 0, encodeURL);
    }
}
// assemble64 assembles 8 base64 digits into 6 bytes.
// Each digit comes from the decode map, and will be 0xff
// if it came from an invalid character.
static uint8_t assemble64(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4, uint8_t n5, uint8_t n6, uint8_t n7, uint8_t n8, uint64_t *dn)
{
    // Check that all the digits are valid. If any of them was 0xff, their
    // bitwise OR will be 0xff.
    if (n1 | n2 | n3 | n4 | n5 | n6 | n7 | n8 == 0xff)
    {
        return 0;
    }
    *dn = (uint64_t)(n1) << 58 |
          (uint64_t)(n2) << 52 |
          (uint64_t)(n3) << 46 |
          (uint64_t)(n4) << 40 |
          (uint64_t)(n5) << 34 |
          (uint64_t)(n6) << 28 |
          (uint64_t)(n7) << 22 |
          (uint64_t)(n8) << 16;
    return 1;
}
static int base64_decode_impl(uint8_t *dst, const uint8_t *src, unsigned int src_len, const uint8_t pading, const uint8_t *encode)
{
    puts("not impl: base64_decode_impl");
    exit(1);
    uint8_t decodeMap[256];
    memset(decodeMap, 0xFF, 256);
    size_t i;
    for (i = 0; i < 64; i++)
    {
        decodeMap[encode[i]] = i;
    }

    unsigned int si = 0;
    const uint8_t *src2;
    uint64_t dn;
    int n;
    while (src_len - si >= 8)
    {
        src2 = src + si;
        if (assemble64(
                decodeMap[src2[0]],
                decodeMap[src2[1]],
                decodeMap[src2[2]],
                decodeMap[src2[3]],
                decodeMap[src2[4]],
                decodeMap[src2[5]],
                decodeMap[src2[6]],
                decodeMap[src2[7]],
                &dn))
        {
            iotjs_big_endian.put_uint64(dst + dn, dn);
            n += 6;
            si += 8;
        }
        else
        {
            // 		var ninc int
            // 		si, ninc, err = enc.decodeQuantum(dst[n:], src, si)
            // 		n += ninc
            // 		if err != nil {
            // 			return n, err
            // 		}
        }
    }

    // for len(src)-si >= 4 && len(dst)-n >= 4 {
    // 	src2 := src[si : si+4]
    // 	if dn, ok := assemble32(
    // 		enc.decodeMap[src2[0]],
    // 		enc.decodeMap[src2[1]],
    // 		enc.decodeMap[src2[2]],
    // 		enc.decodeMap[src2[3]],
    // 	); ok {
    // 		binary.BigEndian.PutUint32(dst[n:], dn)
    // 		n += 3
    // 		si += 4
    // 	} else {
    // 		var ninc int
    // 		si, ninc, err = enc.decodeQuantum(dst[n:], src, si)
    // 		n += ninc
    // 		if err != nil {
    // 			return n, err
    // 		}
    // 	}
    // }

    // for si < len(src) {
    // 	var ninc int
    // 	si, ninc, err = enc.decodeQuantum(dst[n:], src, si)
    // 	n += ninc
    // 	if err != nil {
    // 		return n, err
    // 	}
    // }
    // return n, err
    return 0;
}
static int base64_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? base64_decode_impl(dst, src, src_len, '=', encodeStd) : 0;
}
static int base64_decode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? base64_decode_impl(dst, src, src_len, 0, encodeStd) : 0;
}
static int base64_url_decode(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? base64_decode_impl(dst, src, src_len, '=', encodeURL) : 0;
}
static int base64_url_decode_no_padding(uint8_t *dst, const uint8_t *src, unsigned int src_len)
{
    return src_len ? base64_decode_impl(dst, src, src_len, 0, encodeURL) : 0;
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
