#include <iotjs/modules/native_http.h>
#include <iotjs/modules/crypto_hash.h>
#include <iotjs/modules/net.h>
#include <iotjs/core/configure.h>
#include <iotjs/core/memory.h>
#include <http_parser.h>
#include <stdarg.h>
#include <iotjs/core/binary.h>

#define IOTJS_NET_EXPAND_WS_SUCCESS 0
#define IOTJS_NET_EXPAND_WS_TIMEOUT 10
#define IOTJS_NET_EXPAND_WS_ERROR 11
#define IOTJS_NET_EXPAND_WS_EOF 12

#define IOTJS_NET_EXPAND_WS_STATE_ERR 0x1
#define IOTJS_NET_EXPAND_WS_STATE_HEADERS_COMPLETE 0x2
#define IOTJS_NET_EXPAND_WS_STATE_UPGRADE 0x4
#define IOTJS_NET_EXPAND_WS_STATE_CONNECTION 0x8
#define IOTJS_NET_EXPAND_WS_STATE_SEC_WEBSOCKET_ACCEPT 0x10
#define IOTJS_NET_EXPAND_WS_STATE_READY 0x20

// 解幀的各種狀態
#define IOTJS_NET_EXPAND_WS_FRAME 0        // 開始等待接收最前2字節
#define IOTJS_NET_EXPAND_WS_FRAME_LENGTH 1 // 等待接收 擴展長度 2 or 8 字節
// #define IOTJS_NET_EXPAND_WS_FRAME_MASK 2 // 接收掩碼，僅服務器
#define IOTJS_NET_EXPAND_WS_FRAME_DATA 3 // 接收數據長度
// 將解幀狀態移動到下一步
#define IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, n, to_step) \
    expand->ws.step = to_step;                                  \
    evbuffer_ptr_set(buf, &expand->pos, n, EVBUFFER_PTR_ADD);

#define IOTJS_WS_GET_FRAME_FIN(header) (header[0] & 0x80)
#define IOTJS_WS_GET_FRAME_RSV1(header) (header[0] & 0x40)
#define IOTJS_WS_GET_FRAME_RSV2(header) (header[0] & 0x20)
#define IOTJS_WS_GET_FRAME_RSV3(header) (header[0] & 0x10)
#define IOTJS_WS_GET_FRAME_OPCODE(header) (header[0] & 0xf)
#define IOTJS_WS_GET_FRAME_MASK(header) (header[1] & 0x80)
#define IOTJS_WS_GET_FRAME_MASK_EXTENDED_LENGTH(header) (header[1] & 0x7f)

#define IOTJS_NET_EXPAND_WS_FRAME_FIN(expand) IOTJS_WS_GET_FRAME_FIN(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_RSV1(expand) IOTJS_WS_GET_FRAME_RSV1(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_RSV2(expand) IOTJS_WS_GET_FRAME_RSV2(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_RSV3(expand) IOTJS_WS_GET_FRAME_RSV3(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_OPCODE(expand) IOTJS_WS_GET_FRAME_OPCODE(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_MASK(expand) IOTJS_WS_GET_FRAME_MASK(expand->ws.header)
#define IOTJS_NET_EXPAND_WS_FRAME_MASK_EXTENDED_LENGTH(expand) IOTJS_WS_GET_FRAME_MASK_EXTENDED_LENGTH(expand->ws.header)

#define WEBSOCKET_TEXT_MESSAGE 0x1
#define WEBSOCKET_BINARY_MESSAGE 0x2
#define WEBSOCKET_CLOSE_MESSAGE 0x8
#define WEBSOCKET_PING_MESSAGE 0x9
#define WEBSOCKET_PONG_MESSAGE 0x10

static void native_http_parse_url_field(duk_context *ctx, const char *url, struct http_parser_url *u, const char *field, duk_size_t field_len, const uint16_t index)
{
    if (!(u->field_set & (1 << index)))
    {
        return;
    }
    if (index == UF_PORT)
    {
        duk_push_number(ctx, u->port);
    }
    else
    {
        size_t len = u->field_data[index].len;
        const char *s = url + u->field_data[index].off;
        duk_push_lstring(ctx, s, len);
    }
    duk_put_prop_lstring(ctx, -2, field, field_len);
}
static duk_ret_t native_http_parse_url(duk_context *ctx)
{
    duk_size_t sz;
    const char *s = duk_require_lstring(ctx, 0, &sz);

    struct http_parser_url u;
    http_parser_url_init(&u);
    if (http_parser_parse_url(s, sz, 0, &u))
    {
        duk_pop(ctx);
        duk_error(ctx, DUK_ERR_ERROR, "not a valid url");
        return 0;
    }
    duk_push_object(ctx);
    native_http_parse_url_field(ctx, s, &u, "scheme", 6, UF_SCHEMA);
    native_http_parse_url_field(ctx, s, &u, "userinfo", 4, UF_USERINFO);
    native_http_parse_url_field(ctx, s, &u, "host", 4, UF_HOST);
    native_http_parse_url_field(ctx, s, &u, "port", 4, UF_PORT);
    native_http_parse_url_field(ctx, s, &u, "path", 4, UF_PATH);
    native_http_parse_url_field(ctx, s, &u, "query", 5, UF_QUERY);
    native_http_parse_url_field(ctx, s, &u, "fragement", 9, UF_FRAGMENT);
    return 1;
}
static duk_ret_t native_ws_key(duk_context *ctx)
{
    duk_uint8_t *key = duk_push_buffer(ctx, 16, 0);
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < 16; i++)
    {
        key[i] = rand() % 255;
    }
    duk_base64_encode(ctx, -1);
    return 1;
}
typedef struct
{
    // 消息解析進度
    uint8_t step;
    // 幀頭
    uint8_t header[2 + 8];
    // 當前消息類型
    uint8_t opcode;
    // 幀數據大小
    uint64_t length;
    // 幀長度記錄
    uint8_t extened_length;
} http_expand_ws_frame_t;
typedef struct
{
    http_parser parser;
    union
    {
        http_parser_settings settings;
        http_expand_ws_frame_t ws;
    };

    struct evbuffer_ptr pos;
    union
    {
        size_t n;
        size_t readable;
    };
    union
    {
        // 用於解析 http header
        size_t header;
        //  當前消息長度
        size_t length;
    };
    size_t state;
    // Sec-WebSocket-Key
    const char *key;
    // 最大小消息長度
    size_t limit;
} http_expand_ws_t;
static duk_ret_t native_expand_ws_ec(duk_context *ctx)
{
    tcp_connection_t *conn = duk_require_pointer(ctx, 0);
    duk_int_t err = -1989;
    if (duk_is_number(ctx, 1))
    {
        err = duk_require_int(ctx, 1);
        duk_pop_2(ctx);
    }
    else
    {
        duk_remove(ctx, 0);
    }
    vm_restore(ctx, VM_SNAPSHOT_TCPCONN, conn, 0);

    duk_get_prop_lstring(ctx, -1, "cb", 2);
    duk_swap_top(ctx, -2);
    duk_del_prop_lstring(ctx, -1, "cb", 2);
    duk_pop(ctx);
    switch (err)
    {
    case IOTJS_NET_EXPAND_WS_SUCCESS:
        duk_call(ctx, 0);
        return 0;
    case -1989:
        duk_swap_top(ctx, -2);
        break;
    case IOTJS_NET_EXPAND_WS_TIMEOUT:
        duk_push_lstring(ctx, "reading timeout", 15);
        break;
    case IOTJS_NET_EXPAND_WS_ERROR:
        duk_push_string(ctx, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        break;
    case IOTJS_NET_EXPAND_WS_EOF:
        duk_push_lstring(ctx, "unexpected eof", 14);
        break;
    default:
        duk_push_lstring(ctx, "unknow error", 12);
        break;
    }
    duk_call(ctx, 1);
    return 0;
}
static void expand_ws_ec(tcp_connection_t *conn, duk_int_t err)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_expand_ws_ec, 2, 2, 0);
    duk_push_pointer(ctx, conn);
    duk_push_int(ctx, err);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void expand_ws_lerror(tcp_connection_t *conn, const char *err, duk_size_t len)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_expand_ws_ec, 2, 2, 0);
    duk_push_pointer(ctx, conn);
    duk_push_lstring(ctx, err, len);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void expand_ws_error_pushed(tcp_connection_t *conn)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_pointer(ctx, conn);
    duk_push_c_lightfunc(ctx, native_expand_ws_ec, 2, 2, 0);
    duk_swap_top(ctx, -3);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void expand_ws_error(tcp_connection_t *conn, const char *err)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_expand_ws_ec, 2, 2, 0);
    duk_push_pointer(ctx, conn);
    duk_push_string(ctx, err);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void http_expand_ws_free(void *p)
{
#ifdef VM_TRACE_FINALIZER
    puts("http_expand_ws_free");
#endif
    vm_free(p);
}
static int on_ws_status(http_parser *parser, const char *at, size_t length)
{
    if (parser->status_code == 101)
    {
        return 0;
    }
    tcp_connection_t *conn = parser->data;
    http_expand_ws_t *expand = conn->expand;
    expand->state |= IOTJS_NET_EXPAND_WS_STATE_ERR;
    if (length)
    {
        duk_push_lstring(conn->vm->ctx, at, length);
    }
    else
    {
        duk_push_string(conn->vm->ctx, http_status_str(parser->status_code));
    }
    return 1;
}
static int on_ws_header_field(http_parser *parser, const char *at, size_t length)
{
    tcp_connection_t *conn = parser->data;
    http_expand_ws_t *expand = conn->expand;
    expand->header = 0;
    switch (length)
    {
    case 7:
        if (!memcmp(at, "Upgrade", 7))
        {
            expand->header = IOTJS_NET_EXPAND_WS_STATE_UPGRADE;
        }
        break;
    case 10:
        if (!memcmp(at, "Connection", 10))
        {
            expand->header = IOTJS_NET_EXPAND_WS_STATE_CONNECTION;
        }
        break;
    case 20:
        if (!memcmp(at, "Sec-WebSocket-Accept", 20))
        {
            expand->header = IOTJS_NET_EXPAND_WS_STATE_SEC_WEBSOCKET_ACCEPT;
        }
        break;
    }
    return 0;
}
static BOOL ws_Sec_WebSocket_Accept(tcp_connection_t *conn, const char *s)
{
    http_expand_ws_t *expand = conn->expand;
    // key + 258EAFA5-E914-47DA-95CA-C5AB0DC85B11
    char key[24 + 36] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 53, 56, 69, 65, 70, 65, 53, 45, 69, 57, 49, 52, 45, 52, 55, 68, 65, 45, 57, 53, 67, 65, 45, 67, 53, 65, 66, 48, 68, 67, 56, 53, 66, 49, 49};
    memcpy(key, expand->key, 24);

    // hash_state md;
    if (iotjs_module_crypto_sha1(key, 60, key))
    {
        return FALSE;
    }
    iotjs_base64.std.encode(key + 20, key, 20);
    if (memcmp(s, key + 20, 28))
    {
        return FALSE;
    }
    return TRUE;
}
static int on_ws_header_value(http_parser *parser, const char *at, size_t length)
{
    tcp_connection_t *conn = parser->data;
    http_expand_ws_t *expand = conn->expand;
    switch (expand->header)
    {
    case IOTJS_NET_EXPAND_WS_STATE_UPGRADE:
        if (length != 9 || memcmp(at, "websocket", length))
        {
            expand->state |= IOTJS_NET_EXPAND_WS_STATE_ERR;
            duk_push_lstring(conn->vm->ctx, "Upgrade must be websocket", 25);
            return 1;
        }
        expand->state |= IOTJS_NET_EXPAND_WS_STATE_UPGRADE;
        break;
    case IOTJS_NET_EXPAND_WS_STATE_CONNECTION:
        if (length != 7 || memcmp(at, "Upgrade", length))
        {

            expand->state |= IOTJS_NET_EXPAND_WS_STATE_ERR;
            duk_push_lstring(conn->vm->ctx, "Connection must be Upgrade", 26);
            return 1;
        }
        expand->state |= IOTJS_NET_EXPAND_WS_STATE_CONNECTION;
        break;
    case IOTJS_NET_EXPAND_WS_STATE_SEC_WEBSOCKET_ACCEPT:
        if (length == 28 && ws_Sec_WebSocket_Accept(conn, at))
        {
            expand->state |= IOTJS_NET_EXPAND_WS_STATE_SEC_WEBSOCKET_ACCEPT;
        }
        else
        {
            expand->state |= IOTJS_NET_EXPAND_WS_STATE_ERR;
            duk_push_lstring(conn->vm->ctx, "Sec_WebSocket_Accept not matched", 32);
            return 1;
        }
        break;
    }
    return 0;
}
static int on_ws_headers_complete(http_parser *parser)
{
    tcp_connection_t *conn = parser->data;
    http_expand_ws_t *expand = conn->expand;
    expand->state |= IOTJS_NET_EXPAND_WS_STATE_HEADERS_COMPLETE;
    return 0;
}
static duk_ret_t native_ws_notify_error(duk_context *ctx)
{
    tcp_connection_t *conn = duk_require_pointer(ctx, 1);
    duk_pop(ctx);
    vm_restore(ctx, VM_SNAPSHOT_TCPCONN, conn, 1);
    duk_get_prop_lstring(ctx, -1, "onError", 7);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_swap_top(ctx, -2);
    duk_call(ctx, 1);
    return 0;
}
static void ws_expand_notify_lerror(tcp_connection_t *conn, const char *err, duk_size_t n)
{
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_ws_notify_error, 2, 2, 0);
    duk_push_lstring(ctx, err, n);
    duk_push_pointer(ctx, conn);
    duk_call(ctx, 2);
    duk_pop(ctx);
}

static void ws_expand_notify_errorf(tcp_connection_t *conn, const char *fmt, ...)
{
    va_list ap;
    duk_context *ctx = conn->vm->ctx;
    duk_push_c_lightfunc(ctx, native_ws_notify_error, 2, 2, 0);
    va_start(ap, fmt);
    duk_push_vsprintf(ctx, fmt, ap);
    va_end(ap);
    duk_push_pointer(ctx, conn);
    duk_call(ctx, 2);
    duk_pop(ctx);
}
static void ws_expand_read_ws_cb_print_header(http_expand_ws_t *expand)
{
    printf("%d\n", expand->ws.header[0]);
    printf("FIN: %d\n", IOTJS_NET_EXPAND_WS_FRAME_FIN(expand) ? 1 : 0);
    printf("RSV1: %d\n", IOTJS_NET_EXPAND_WS_FRAME_RSV1(expand) ? 1 : 0);
    printf("RSV2: %d\n", IOTJS_NET_EXPAND_WS_FRAME_RSV2(expand) ? 1 : 0);
    printf("RSV3: %d\n", IOTJS_NET_EXPAND_WS_FRAME_RSV3(expand) ? 1 : 0);
    printf("opcode: %d\n", IOTJS_NET_EXPAND_WS_FRAME_OPCODE(expand));
    printf("Mask: %d\n", IOTJS_NET_EXPAND_WS_FRAME_MASK(expand) ? 1 : 0);
    size_t n = IOTJS_NET_EXPAND_WS_FRAME_MASK_EXTENDED_LENGTH(expand);
    switch (n)
    {
    case 126:
        puts("Extended payload length: 2");
        break;
    case 127:
        puts("Extended payload length: 8");
        break;
    default:
        puts("Extended payload length: 0");
        break;
    }
    printf("payload length: %ld\n", expand->ws.length);
}

static uint8_t ws_expand_read_ws_cb_frame(tcp_connection_t *conn, http_expand_ws_t *expand, struct evbuffer *buf)
{
    evbuffer_copyout_from(buf, &expand->pos, expand->ws.header, 2);
    // ws_expand_read_ws_cb_print_header(expand);
    if (IOTJS_NET_EXPAND_WS_FRAME_RSV1(expand))
    {
        ws_expand_notify_lerror(conn, "RSV1 set", 8);
        return 1;
    }
    if (IOTJS_NET_EXPAND_WS_FRAME_RSV2(expand))
    {
        ws_expand_notify_lerror(conn, "RSV2 set", 8);
        return 1;
    }
    if (IOTJS_NET_EXPAND_WS_FRAME_RSV3(expand))
    {
        ws_expand_notify_lerror(conn, "RSV3 set", 8);
        return 1;
    }
    // 服務器發送的數據不能設置 mask
    if (IOTJS_NET_EXPAND_WS_FRAME_MASK(expand))
    {
        ws_expand_notify_lerror(conn, "bad MASK", 8);
        return 1;
    }

    duk_uint8_t opcode = IOTJS_NET_EXPAND_WS_FRAME_OPCODE(expand);
    duk_uint8_t extened_length = IOTJS_NET_EXPAND_WS_FRAME_MASK_EXTENDED_LENGTH(expand);
    switch (opcode)
    {
    case 0: // 延續上一幀
    case WEBSOCKET_TEXT_MESSAGE:
    case WEBSOCKET_BINARY_MESSAGE:
        expand->ws.opcode = opcode;
        // 進入下一步驟
        if (extened_length < 126)
        {
            expand->ws.extened_length = 0;
            expand->ws.length = extened_length;

            expand->length += expand->ws.length;
            if (expand->limit > 0 && expand->length > expand->limit)
            {
                ws_expand_notify_lerror(conn, "message too large", 17);
                return 1;
            }
            IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, 2, IOTJS_NET_EXPAND_WS_FRAME_DATA)
        }
        else
        {
            expand->ws.extened_length = extened_length == 126 ? 2 : 8;
            IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, 2, IOTJS_NET_EXPAND_WS_FRAME_LENGTH)
        }
        break;
    case WEBSOCKET_CLOSE_MESSAGE:
    case WEBSOCKET_PING_MESSAGE:
    case WEBSOCKET_PONG_MESSAGE:
        expand->ws.opcode = opcode;
        // 控制幀 數據最大只能是 125
        if (extened_length > 125)
        {
            ws_expand_notify_lerror(conn, "len > 125 for control", 21);
            return 1;
        }
        // 控制幀只能是單幀
        if (!IOTJS_NET_EXPAND_WS_FRAME_FIN(expand))
        {
            ws_expand_notify_lerror(conn, "FIN not set on control", 22);
            return 1;
        }
        // 進入下一步驟
        expand->ws.extened_length = 0;
        expand->ws.length = extened_length;
        IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, 2, IOTJS_NET_EXPAND_WS_FRAME_DATA)
        break;
    default:
        ws_expand_notify_errorf(conn, "bad opcode %d", opcode);
        return 1;
    }
    return 0;
}
static uint8_t ws_expand_read_ws_cb_length(tcp_connection_t *conn, http_expand_ws_t *expand, struct evbuffer *buf)
{
    uint8_t *b = expand->ws.header + 2;
    evbuffer_copyout_from(buf, &expand->pos, b, expand->ws.extened_length);
    if (expand->ws.extened_length == 2)
    {
        expand->ws.length = iotjs_big_endian.uint16(b);
    }
    else
    {
        expand->ws.length = iotjs_big_endian.uint64(b);
    }

    expand->length += expand->ws.length;
    if (expand->limit > 0 && expand->length > expand->limit)
    {
        ws_expand_notify_lerror(conn, "message too large", 17);
        return 1;
    }
    IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, expand->ws.extened_length, IOTJS_NET_EXPAND_WS_FRAME_DATA)
    return 0;
}

static uint8_t ws_expand_read_ws_cb_data(tcp_connection_t *conn, http_expand_ws_t *expand, struct evbuffer *buf)
{
    // ws_expand_read_ws_cb_print_header(expand);
    if (!IOTJS_NET_EXPAND_WS_FRAME_FIN(expand))
    {
        // 還有後續幀，繼續接收
        IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, expand->ws.length, IOTJS_NET_EXPAND_WS_FRAME)
        return 0;
    }
    switch (IOTJS_NET_EXPAND_WS_FRAME_OPCODE(expand))
    {
    case WEBSOCKET_CLOSE_MESSAGE:
    case WEBSOCKET_PING_MESSAGE:
    case WEBSOCKET_PONG_MESSAGE:
        // 通知收到控制幀

        // 處理完成後繼續接收下一幀
        if (expand->readable || expand->length)
        {
            IOTJS_NET_EXPAND_WS_FRAME_NEXT(expand, buf, expand->ws.length, IOTJS_NET_EXPAND_WS_FRAME)
        }
        else
        {
            // 沒有積壓的數據直接刪除控制幀
            evbuffer_drain(buf, expand->pos.pos + expand->ws.length);
            evbuffer_ptr_set(buf, &expand->pos, 0, EVBUFFER_PTR_SET);
            expand->ws.step = IOTJS_NET_EXPAND_WS_FRAME;
        }
        return 0;
    }

    // 設置設備可讀
    expand->readable = 1;

    // 通知設備可讀
    duk_context *ctx = conn->vm->ctx;
    duk_context *snapshot = vm_require_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn);
    duk_push_lstring(snapshot, "onRead", 6);
    duk_call_prop(snapshot, 0, 0);
    duk_pop(snapshot);
    return 1;
}
static void ws_expand_read_ws_cb(tcp_connection_t *conn, http_expand_ws_t *expand, struct evbuffer *buf)
{
    evbuffer_ptr_set(buf, &expand->pos, expand->pos.pos, EVBUFFER_PTR_SET);
    while (1)
    {
        // 未處理數據長度
        size_t length = evbuffer_get_length(buf) - expand->pos.pos;
        switch (expand->ws.step)
        {
        case IOTJS_NET_EXPAND_WS_FRAME:
            if (length < 2 || ws_expand_read_ws_cb_frame(conn, expand, buf))
            {
                return;
            }
            break;
        case IOTJS_NET_EXPAND_WS_FRAME_LENGTH:
            if (length < expand->ws.extened_length || ws_expand_read_ws_cb_length(conn, expand, buf))
            {
                return;
            }
            break;
        case IOTJS_NET_EXPAND_WS_FRAME_DATA:
            if (length < expand->ws.length || ws_expand_read_ws_cb_data(conn, expand, buf))
            {
                return;
            }
            break;
        }
    }
}
static void ws_expand_read_cb(struct bufferevent *bev, void *args)
{
    tcp_connection_t *conn = args;
    http_expand_ws_t *expand = conn->expand;
    struct evbuffer *buf = bufferevent_get_input(conn->bev);
    if (expand->state & IOTJS_NET_EXPAND_WS_STATE_READY)
    {
        if (expand->readable)
        {
            // 有未讀取的完整消息，通知設備可讀
            duk_context *ctx = conn->vm->ctx;
            duk_context *snapshot = vm_require_snapshot(ctx, VM_SNAPSHOT_TCPCONN, conn);
            duk_push_lstring(snapshot, "onRead", 6);
            duk_call_prop(snapshot, 0, 0);
            duk_pop(snapshot);
        }
        else
        {
            // 沒有完整消息接解析數據
            ws_expand_read_ws_cb(conn, expand, buf);
        }
    }
    else
    {
        // 尋找第一個換行
        if (!expand->n)
        {
            expand->pos = evbuffer_search_eol(buf, NULL, &expand->n, EVBUFFER_EOL_CRLF);
            if (expand->pos.pos < 0)
            {
                return;
            }
            evbuffer_ptr_set(buf, &expand->pos, expand->n, EVBUFFER_PTR_ADD);
        }
        // 尋找空行
        struct evbuffer_ptr skip = expand->pos;
        size_t n;
        while (1)
        {
            expand->pos = evbuffer_search_eol(buf, &skip, &n, EVBUFFER_EOL_CRLF);
            if (expand->pos.pos < 0)
            {
                return;
            }
            expand->n = n;
            if (skip.pos != expand->pos.pos)
            {
                skip = expand->pos;
                evbuffer_ptr_set(buf, &skip, expand->n, EVBUFFER_PTR_ADD);
                continue;
            }
            n = expand->pos.pos + expand->n;
            break;
        }
        // 線性化 http header，通常本身就是線性化的所以幾乎沒有開銷
        unsigned char *s = evbuffer_pullup(buf, n);
        if (!s)
        {
            expand_ws_lerror(conn, "evbuffer_pullup fail", 20);
            return;
        }
        size_t readed = http_parser_execute(&expand->parser, &expand->settings, s, n);
        http_parser_execute(&expand->parser, &expand->settings, s, 0);
        if (expand->state & IOTJS_NET_EXPAND_WS_STATE_ERR)
        {
            expand_ws_error_pushed(conn);
            return;
        }
        else if (expand->parser.http_errno != HPE_OK)
        {
            expand_ws_error(conn, http_errno_description(expand->parser.http_errno));
            return;
        }
        else if (readed != n)
        {
            expand_ws_lerror(conn, "http parse short", 16);
            return;
        }
        else if (!(expand->state & IOTJS_NET_EXPAND_WS_STATE_HEADERS_COMPLETE))
        {
            expand_ws_lerror(conn, "http parse header not complete", 30);
            return;
        }
        else if (!(expand->state & IOTJS_NET_EXPAND_WS_STATE_SEC_WEBSOCKET_ACCEPT))
        {
            expand_ws_lerror(conn, "http parse header not found Sec-WebSocket-Accept", 48);
            return;
        }
        else if (!(expand->state & IOTJS_NET_EXPAND_WS_STATE_UPGRADE))
        {
            expand_ws_lerror(conn, "http parse header not found Upgrade", 35);
            return;
        }
        else if (!(expand->state & IOTJS_NET_EXPAND_WS_STATE_CONNECTION))
        {
            expand_ws_lerror(conn, "http parse header not found Connection", 38);
            return;
        }
        else if (expand->parser.content_length && expand->parser.content_length != (uint64_t)-1)
        {
            expand_ws_lerror(conn, "upgrade websocket not support content", 37);
            return;
        }
        expand_ws_ec(conn, 0);
        // 初始化 解幀狀態
        expand->state = IOTJS_NET_EXPAND_WS_STATE_READY;
        expand->ws.length = 0;
        expand->ws.step = IOTJS_NET_EXPAND_WS_FRAME;

        expand->pos.pos = 0;
        expand->length = 0;
        expand->readable = 0;

        bufferevent_setcb(conn->bev, ws_expand_read_cb, tcp_connection_write_cb, tcp_connection_event_cb, conn);
        evbuffer_drain(buf, readed);
    }
}
static void ws_expand_event_cb(struct bufferevent *bev, short what, void *args)
{
    tcp_connection_t *conn = args;
    if (what & BEV_EVENT_EOF)
    {
        expand_ws_ec(args, IOTJS_NET_EXPAND_WS_EOF);
    }
    else if (what & BEV_EVENT_TIMEOUT)
    {
        expand_ws_ec(args, IOTJS_NET_EXPAND_WS_TIMEOUT);
    }
    else if (what & BEV_EVENT_ERROR)
    {
        expand_ws_ec(args, IOTJS_NET_EXPAND_WS_ERROR);
    }
}
static duk_ret_t native_http_expand_ws(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *conn = finalizer->p;
    duk_require_callable(ctx, 3);
    duk_put_prop_lstring(ctx, 0, "cb", 2);
    size_t limit = duk_require_number(ctx, 2);
    duk_pop(ctx);
    const char *key = duk_require_lstring(ctx, 1, NULL);
    duk_put_prop_lstring(ctx, 0, "_key", 4);
    http_expand_ws_t *expand = vm_malloc(sizeof(http_expand_ws_t));
    if (!expand)
    {
        duk_error(ctx, DUK_ERR_ERROR, "malloc http_expand_ws_t fail");
    }
    memset(expand, 0, sizeof(http_expand_ws_t));
    conn->expand_free = http_expand_ws_free;
    conn->expand = expand;
    expand->key = key;
    expand->limit = limit;

    http_parser_init(&expand->parser, HTTP_RESPONSE);
    http_parser_settings_init(&expand->settings);
    expand->parser.data = conn;

    // 設置回調
    // expand->settings.on_message_begin = on_ws_message_begin; // 開始解析消息
    expand->settings.on_status = on_ws_status;             // status 字符串
    expand->settings.on_header_field = on_ws_header_field; // field 和 value 會被交替調用
    expand->settings.on_header_value = on_ws_header_value;
    expand->settings.on_headers_complete = on_ws_headers_complete; // headers 解析完成後回調
                                                                   // 注意 要判斷 parser->content_length 是否爲0
    // expand->settings.on_chunk_header = on_ws_chunk_header;         // 讀取到 chunk 頭時回調 此時 parser->content_length 記錄了header 長度
    // expand->settings.on_chunk_complete = on_ws_chunk_complete;     // 每當一個 chunk 被讀完時回調
    // // 對於 Content-Length 會回調一次
    // // 對於 Transfer-Encoding: chunked 會回調多次 每次傳入 chunk 數據偏移at 和 長度length
    // expand->settings.on_body = on_ws_body;                         // 接收到一個 body
    // expand->settings.on_message_complete = on_ws_message_complete; // 消息接收完成後回調

    bufferevent_setcb(conn->bev, ws_expand_read_cb, NULL, ws_expand_event_cb, conn);
    return 0;
}
static duk_ret_t native_ws_readable(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    duk_pop(ctx);
    tcp_connection_t *conn = finalizer->p;
    http_expand_ws_t *expand = conn->expand;
    if (expand->readable)
    {
        duk_push_true(ctx);
    }
    else
    {
        duk_push_false(ctx);
    }
    return 1;
}
static duk_ret_t native_ws_read(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    duk_pop(ctx);
    tcp_connection_t *conn = finalizer->p;
    http_expand_ws_t *expand = conn->expand;
    if (!expand->readable)
    {
        // 沒有可讀數據字節返回
        return 0;
    }

    // 讀取數據
    uint8_t *dst = duk_push_buffer(ctx, expand->length, 0);
    struct evbuffer *buf = bufferevent_get_input(conn->bev);
    size_t length;
    uint8_t opcode;
    uint8_t control;
    uint8_t header[10];
    uint8_t mask[4];
    uint8_t masked;
    size_t i;
    while (1)
    {
        evbuffer_remove(buf, header, 2);
        length = IOTJS_WS_GET_FRAME_MASK_EXTENDED_LENGTH(header);
        control = 0;
        switch (IOTJS_WS_GET_FRAME_OPCODE(header))
        {
        case 0:
            break;
        case WEBSOCKET_BINARY_MESSAGE:
            opcode = WEBSOCKET_BINARY_MESSAGE;
            break;
        case WEBSOCKET_TEXT_MESSAGE:
            opcode = WEBSOCKET_TEXT_MESSAGE;
            break;
        default:
            control = 1;
            break;
        }
        // 是否存在掩碼
        masked = IOTJS_WS_GET_FRAME_MASK(header);
        if (control)
        {
            // 控制幀字直接刪除
            if (masked)
            {
                length += 4;
            }
            if (length)
            {
                evbuffer_drain(buf, length);
            }
            continue;
        }
        switch (length)
        {
        case 126:
            evbuffer_remove(buf, header + 2, 2);
            length = iotjs_big_endian.uint16(header + 2);
            break;
        case 127:
            evbuffer_remove(buf, header + 2, 8);
            length = iotjs_big_endian.uint64(header + 2);
            break;
        }
        if (masked)
        {
            evbuffer_remove(buf, mask, 4);
        }
        if (length)
        {
            evbuffer_remove(buf, dst, length);
            if (masked)
            {
                for (i = 0; i < length; i++)
                {
                    dst[i] ^= mask[i % 4];
                }
            }
            dst += length;
        }
        if (IOTJS_WS_GET_FRAME_FIN(header))
        {
            break;
        }
    }
    if (opcode == WEBSOCKET_TEXT_MESSAGE)
    {
        duk_buffer_to_string(ctx, -1);
    }

    //  重置解幀狀態
    expand->ws.length = 0;
    expand->ws.step = IOTJS_NET_EXPAND_WS_FRAME;
    expand->length = 0;
    expand->readable = 0;
    evbuffer_ptr_set(buf, &expand->pos, 0, EVBUFFER_PTR_SET);

    if (evbuffer_get_contiguous_space(buf))
    {
        // 存在未處理數據，觸發可讀回調
        bufferevent_trigger(conn->bev, EV_READ, BEV_OPT_DEFER_CALLBACKS);
    }
    return 1;
}

static duk_ret_t native_ws_send(duk_context *ctx)
{
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    duk_swap_top(ctx, -2);
    duk_pop(ctx);
    duk_size_t sz;
    const char *src;
    uint8_t s[2 + 8];
    if (duk_is_string(ctx, -1))
    {
        s[0] = 0x80 | WEBSOCKET_TEXT_MESSAGE;
        src = duk_require_lstring(ctx, -1, &sz);
    }
    else
    {
        s[0] = 0x80 | WEBSOCKET_BINARY_MESSAGE;
        src = duk_require_buffer_data(ctx, -1, &sz);
    }
    char *tmp = NULL;
    if (sz)
    {
        tmp = duk_push_buffer(ctx, sz, 0);
    }
    tcp_connection_t *conn = finalizer->p;
    http_expand_ws_t *expand = conn->expand;
    struct evbuffer *buf = bufferevent_get_output(conn->bev);
    uint64_t write = 2 + 4;
    uint8_t extend_length;
    if (sz > IOTJS_MAX_UINT16)
    {
        extend_length = 8;
        write += 8;
        s[1] = 0x80 | 127;
        iotjs_big_endian.put_uint64(s + 2, sz);
    }
    else if (sz > 125)
    {
        extend_length = 2;
        write += 2;
        s[1] = 0x80 | 126;
        iotjs_big_endian.put_uint16(s + 2, sz);
    }
    else
    {
        extend_length = 0;
        s[1] = 0x80 | sz;
    }
    size_t len = evbuffer_get_length(buf);

    // 驗證寫入限制
    if (conn->write)
    {
        if (write > conn->write)
        {
            duk_pop_2(ctx);
            duk_push_lstring(ctx, "data too large", 14);
            duk_throw(ctx);
        }
        else if (conn->write - len < write)
        {
            duk_pop_2(ctx);
            duk_push_false(ctx);
            return 1;
        }
    }
    // 擴展緩衝區以便確定寫入不會失敗
    if (evbuffer_expand(buf, write))
    {
        duk_pop_2(ctx);
        duk_push_lstring(ctx, "evbuffer_expand fail", 20);
        duk_throw(ctx);
    }

    // 寫幀
    evbuffer_add(buf, s, 2 + extend_length);
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < 4; i++)
    {
        s[i] = rand() % 255;
    }
    evbuffer_add(buf, s, 4);
    if (tmp)
    {
        for (size_t i = 0; i < sz; i++)
        {
            tmp[i] = src[i] ^ s[i % 4];
        }
        evbuffer_add(buf, tmp, sz);
    }
    duk_pop(ctx);
    duk_push_true(ctx);
    return 1;
}
void native_iotjs_net_deps_http(duk_context *ctx)
{
    duk_push_c_lightfunc(ctx, native_http_parse_url, 1, 1, 0);
    duk_put_prop_lstring(ctx, -2, "http_parse_url", 14);
    duk_push_c_lightfunc(ctx, native_ws_key, 0, 0, 0);
    duk_put_prop_lstring(ctx, -2, "ws_key", 6);
    duk_push_c_lightfunc(ctx, native_http_expand_ws, 4, 4, 0);
    duk_put_prop_lstring(ctx, -2, "http_expand_ws", 14);

    duk_push_c_lightfunc(ctx, native_ws_readable, 1, 1, 0);
    duk_put_prop_lstring(ctx, -2, "ws_readable", 11);
    duk_push_c_lightfunc(ctx, native_ws_read, 1, 1, 0);
    duk_put_prop_lstring(ctx, -2, "ws_read", 7);
    duk_push_c_lightfunc(ctx, native_ws_send, 2, 2, 0);
    duk_put_prop_lstring(ctx, -2, "ws_send", 7);
}