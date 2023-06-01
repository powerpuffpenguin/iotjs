#include <event2/event.h>
#include <event2/dns.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#define TEST_CONNECT_HOSTNAME 1
#ifdef TEST_CONNECT_HOSTNAME
#define TEST_CONNECT_ADDR "localhost"
#else
#define TEST_CONNECT_ADDR "127.0.0.1"
#endif
#define TEST_CONNECT_PORT 9001

#define CLIENT_MAX_MESSAGE (32 * 1024)
#define CLIENT_STEP_CONNECT 0
#define CLIENT_STEP_SEND_HELLO 1
#define CLIENT_STEP_SEND_DATA 2
#define CLIENT_STEP_SEND_COMPLETE 3
typedef struct
{
    struct event *timeout;
    struct bufferevent *bev;
#ifdef TEST_CONNECT_HOSTNAME
    struct evdns_base *esb;
#endif
    int state;
    int step;
    uint16_t header;
    char buffer[CLIENT_MAX_MESSAGE + 1];
    char *message;
    int count;
} client_t;
void client_free(client_t *client)
{
    if (client->bev)
    {
        bufferevent_free(client->bev);
    }
    if (client->timeout)
    {
        event_free(client->timeout);
    }
#ifdef TEST_CONNECT_HOSTNAME
    if (client->esb)
    {
        evdns_base_free(client->esb, 0);
    }
#endif
    free(client);
}
int client_send(client_t *client, const char *str)
{
    size_t len = strlen(str);
    if (len > CLIENT_MAX_MESSAGE)
    {
        puts("message too large");
        return -1;
    }
    uint16_t header = len;
    if (bufferevent_write(client->bev, &header, 2))
    {
        puts("write error");
        return -1;
    }
    if (bufferevent_write(client->bev, str, len))
    {
        puts("write error");
        return -1;
    }
    return 0;
}
int client_loop(client_t *client)
{
    switch (client->step)
    {
    case CLIENT_STEP_CONNECT:
        puts("send hello");
        if (client_send(client, "hello message"))
        {
            break;
        }
        client->step = CLIENT_STEP_SEND_HELLO;
        return 0;
    case CLIENT_STEP_SEND_HELLO:
        if (memcmp(client->message, "hello message", client->header))
        {
            printf("received unexpected hello: %s\n", client->message);
            break;
        }
        if (client_send(client, "data message"))
        {
            break;
        }
        client->count = 1;
        client->step = CLIENT_STEP_SEND_DATA;
        return 0;
    case CLIENT_STEP_SEND_DATA:
        printf("recv %d: %s\n", client->count, client->message);
        client->count++;
        if (client->count < 10) // 繼續模擬數據收發
        {
            if (client_send(client, "data message"))
            {
                break;
            }
            return 0;
        }

        // 工作完成 釋放資源
        client_free(client);
        return 0;
    default:
        printf("unexpected state %d\n", client->step);
        break;
    }
    return -1;
}
void event_read_cb(struct bufferevent *bev, void *ctx)
{
    puts("event_read_cb");
    client_t *client = ctx;
    if (client->message)
    {
        puts("received a message that was too late to process");
        client_free(client);
        return;
    }
    struct evbuffer *buf = bufferevent_get_input(bev);
    size_t recv = evbuffer_get_length(buf);
    // 收包頭
    while (1)
    {
        if (client->header) // 已收到完整包頭
        {
            break;
        }
        else if (recv < 2) // 沒有完整包頭，等待網路數據
        {
            return;
        }
        // 讀取包頭
        evbuffer_remove(buf, &client->header, 2);
        recv -= 2;
    }
    if (client->header > CLIENT_MAX_MESSAGE)
    {
        puts("message too large");
        client_free(client);
        return;
    }
    // 等待一個完整的消息
    if (recv < client->header)
    {
        return;
    }
    evbuffer_remove(buf, client->buffer, client->header);
    client->message = client->buffer;
    client->message[client->header] = 0;
    // 進入狀態機 處理消息
    if (client_loop(client))
    {
        client_free(client);
        return;
    }
    // 處理完成準備接收下一個消息
    client->header = 0;
    client->message = NULL;
}
void event_connect_timeout_cb(evutil_socket_t fd, short events, void *arg)
{
    puts("connect timeout");
    client_free(arg);
}
void event_connect_cb(struct bufferevent *bev, short what, client_t *client)
{
    // 關閉 timeout
    if (client->timeout)
    {
        event_free(client->timeout);
        client->timeout = NULL;
    }

    if (what & BEV_EVENT_CONNECTED)
    {
        puts("connect success");
        client->state = 1;
        // 連接成功 進入連接狀態機
        if (client_loop(client))
        {
            client_free(client);
        }
    }
    else if (what & BEV_EVENT_ERROR)
    {
#ifdef TEST_CONNECT_HOSTNAME
        int err = bufferevent_socket_get_dns_error(bev);
        if (err)
        {
            printf("DNS error: %s\n", evutil_gai_strerror(err));
        }
        else
        {
            printf("connect: %s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        }
#else
        printf("connect: %s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
#endif
        client_free(client);
    }
}
void event_cb(struct bufferevent *bev, short what, void *ctx)
{
    puts("------------event_cb");
    client_t *client = ctx;
    if (!client->state)
    {
        event_connect_cb(bev, what, client);
        return;
    }
    if (what & BEV_EVENT_WRITING)
    {
        if (what & BEV_EVENT_EOF)
        {
            puts("write on eof");
        }
        else if (what & BEV_EVENT_TIMEOUT)
        {
            puts("write timeout");
        }
        else if (what & BEV_EVENT_ERROR)
        {
            printf("write error: %s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        }
    }
    else if (what & BEV_EVENT_READING)
    {
        if (what & BEV_EVENT_EOF)
        {
            puts("read on eof");
        }
        else if (what & BEV_EVENT_TIMEOUT)
        {
            puts("read timeout");
        }
        else if (what & BEV_EVENT_ERROR)
        {
            printf("read error: %s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        }
    }
    client_free(client);
}
int bufferevent_example(struct event_base *eb)
{
    int ret = -1;
    client_t *client = malloc(sizeof(client_t));
    if (!client)
    {
        puts("malloc client error");
        goto END;
    }
    memset(client, 0, sizeof(client_t));
    // 爲連接添加 超時
    client->timeout = event_new(eb, -1, EV_TIMEOUT, event_connect_timeout_cb, client);
    if (!client->timeout)
    {
        puts("event_new connect timeout error");
        goto END;
    }
    struct timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0,
    };
    if (event_add(client->timeout, &tv))
    {
        puts("event_add connect timeout error");
        goto END;
    }

    // 創建 bufferevent
    client->bev = bufferevent_socket_new(eb, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    if (!client->bev)
    {
        puts("bufferevent_socket_new error");
        goto END;
    }
    // 設置回調
    bufferevent_setcb(client->bev, event_read_cb, NULL, event_cb, client);
    // 啓用讀寫
    if (bufferevent_enable(client->bev, EV_READ | EV_WRITE))
    {
        puts("bufferevent_enable error");
        goto END;
    }
#ifdef TEST_CONNECT_HOSTNAME
    client->esb = evdns_base_new(eb, EVDNS_BASE_INITIALIZE_NAMESERVERS | EVDNS_BASE_DISABLE_WHEN_INACTIVE);
    if (!client->esb)
    {
        puts("evdns_base_new error");
        goto END;
    }
    if (bufferevent_socket_connect_hostname(client->bev, client->esb, AF_UNSPEC, TEST_CONNECT_ADDR, TEST_CONNECT_PORT))
    {
        puts("bufferevent_socket_connect error");
        goto END;
    }
#else
    // 連接服務器
    struct in_addr in_addr;
    if (evutil_inet_pton(AF_INET, TEST_CONNECT_ADDR, &in_addr) != 1)
    {
        puts("evutil_inet_pton error");
        goto END;
    }
    struct sockaddr_in addr;
    addr.sin_addr = in_addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_CONNECT_PORT);
    if (bufferevent_socket_connect(client->bev, (const struct sockaddr *)&addr, sizeof(addr)))
    {
        puts("bufferevent_socket_connect error");
        goto END;
    }
#endif
    puts("connecting");

    ret = 0;
    if (!ret)
    {
        return 0;
    }
END:
    if (client)
    {
        client_free(client);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    struct event_base *eb = NULL;
    eb = event_base_new();
    if (!eb)
    {
        puts("event_base_new error");
        goto END;
    }

    if (bufferevent_example(eb))
    {
        goto END;
    }
    ret = event_base_dispatch(eb);
    if (ret < 0)
    {
        puts("event_base_dispatch error");
        goto END;
    }
    ret = 0;
END:
    if (eb)
    {
        event_base_free(eb);
    }
    puts("end");
    return ret;
}