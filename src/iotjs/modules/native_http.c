#include <iotjs/modules/native_http.h>
#include <iotjs/modules/crypto_hash.h>
#include <iotjs/modules/net.h>
#include <iotjs/core/configure.h>
#include <iotjs/core/memory.h>
#include <http_parser.h>

#define IOTJS_NET_expand_WS_SUCCESS 0
#define IOTJS_NET_expand_WS_TIMEOUT 10
#define IOTJS_NET_expand_WS_ERROR 11
#define IOTJS_NET_expand_WS_EOF 12

#define IOTJS_NET_expand_WS_STATE_ERR 0x1
#define IOTJS_NET_expand_WS_STATE_HEADERS_COMPLETE 0x2
#define IOTJS_NET_expand_WS_STATE_UPGRADE 0x4
#define IOTJS_NET_expand_WS_STATE_CONNECTION 0x8
#define IOTJS_NET_expand_WS_STATE_SEC_WEBSOCKET_ACCEPT 0x10
#define IOTJS_NET_expand_WS_STATE_READY 0x20

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
duk_ret_t native_http_parse_url(duk_context *ctx)
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
duk_ret_t native_ws_key(duk_context *ctx)
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
    http_parser parser;
    http_parser_settings settings;
    struct evbuffer_ptr pos;
    size_t n;
    size_t header;
    size_t state;
    const char *key;
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
    case IOTJS_NET_expand_WS_SUCCESS:
        duk_call(ctx, 0);
        return 0;
    case -1989:
        duk_swap_top(ctx, -2);
        break;
    case IOTJS_NET_expand_WS_TIMEOUT:
        duk_push_lstring(ctx, "reading timeout", 15);
        break;
    case IOTJS_NET_expand_WS_ERROR:
        duk_push_string(ctx, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        break;
    case IOTJS_NET_expand_WS_EOF:
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
    expand->state |= IOTJS_NET_expand_WS_STATE_ERR;
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
            expand->header = IOTJS_NET_expand_WS_STATE_UPGRADE;
        }
        break;
    case 10:
        if (!memcmp(at, "Connection", 10))
        {
            expand->header = IOTJS_NET_expand_WS_STATE_CONNECTION;
        }
        break;
    case 20:
        if (!memcmp(at, "Sec-WebSocket-Accept", 20))
        {
            expand->header = IOTJS_NET_expand_WS_STATE_SEC_WEBSOCKET_ACCEPT;
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
    unsigned long outlen;
    if (iotjs_module_crypto_base64_encode(key, 20, key + 20, &outlen))
    {
        return FALSE;
    }
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
    case IOTJS_NET_expand_WS_STATE_UPGRADE:
        if (length != 9 || memcmp(at, "websocket", length))
        {
            expand->state |= IOTJS_NET_expand_WS_STATE_ERR;
            duk_push_lstring(conn->vm->ctx, "Upgrade must be websocket", 25);
            return 1;
        }
        expand->state |= IOTJS_NET_expand_WS_STATE_UPGRADE;
        break;
    case IOTJS_NET_expand_WS_STATE_CONNECTION:
        if (length != 7 || memcmp(at, "Upgrade", length))
        {

            expand->state |= IOTJS_NET_expand_WS_STATE_ERR;
            duk_push_lstring(conn->vm->ctx, "Connection must be Upgrade", 26);
            return 1;
        }
        expand->state |= IOTJS_NET_expand_WS_STATE_CONNECTION;
        break;
    case IOTJS_NET_expand_WS_STATE_SEC_WEBSOCKET_ACCEPT:
        if (length == 28 && ws_Sec_WebSocket_Accept(conn, at))
        {
            expand->state |= IOTJS_NET_expand_WS_STATE_SEC_WEBSOCKET_ACCEPT;
        }
        else
        {
            expand->state |= IOTJS_NET_expand_WS_STATE_ERR;
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
    expand->state |= IOTJS_NET_expand_WS_STATE_HEADERS_COMPLETE;
    return 0;
}
static void ws_expand_read_cb(struct bufferevent *bev, void *args)
{
    tcp_connection_t *conn = args;
    http_expand_ws_t *expand = conn->expand;
    struct evbuffer *buf = bufferevent_get_input(conn->bev);
    if (expand->state == IOTJS_NET_expand_WS_STATE_READY)
    {
        puts("ws_expand_read_cb ----");
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
        if (expand->state & IOTJS_NET_expand_WS_STATE_ERR)
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
        else if (!(expand->state & IOTJS_NET_expand_WS_STATE_HEADERS_COMPLETE))
        {
            expand_ws_lerror(conn, "http parse header not complete", 30);
            return;
        }
        else if (!(expand->state & IOTJS_NET_expand_WS_STATE_SEC_WEBSOCKET_ACCEPT))
        {
            expand_ws_lerror(conn, "http parse header not found Sec-WebSocket-Accept", 48);
            return;
        }
        else if (!(expand->state & IOTJS_NET_expand_WS_STATE_UPGRADE))
        {
            expand_ws_lerror(conn, "http parse header not found Upgrade", 35);
            return;
        }
        else if (!(expand->state & IOTJS_NET_expand_WS_STATE_CONNECTION))
        {
            expand_ws_lerror(conn, "http parse header not found Connection", 38);
            return;
        }
        expand_ws_ec(conn, 0);
        expand->state = IOTJS_NET_expand_WS_STATE_READY;
        bufferevent_setcb(conn->bev, ws_expand_read_cb, tcp_connection_write_cb, tcp_connection_event_cb, conn);
        evbuffer_drain(buf, readed);
    }
}
static void ws_expand_event_cb(struct bufferevent *bev, short what, void *args)
{
    tcp_connection_t *conn = args;
    if (what & BEV_EVENT_EOF)
    {
        expand_ws_ec(args, IOTJS_NET_expand_WS_EOF);
    }
    else if (what & BEV_EVENT_TIMEOUT)
    {
        expand_ws_ec(args, IOTJS_NET_expand_WS_TIMEOUT);
    }
    else if (what & BEV_EVENT_ERROR)
    {
        expand_ws_ec(args, IOTJS_NET_expand_WS_ERROR);
    }
}
duk_ret_t native_http_expand_ws(duk_context *ctx)
{
    vm_dump_context_stdout(ctx);
    finalizer_t *finalizer = vm_require_finalizer(ctx, 0, tcp_connection_free);
    tcp_connection_t *conn = finalizer->p;
    duk_require_callable(ctx, 2);
    duk_put_prop_lstring(ctx, 0, "cb", 2);
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
