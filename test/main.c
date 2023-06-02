#include <event2/event.h>
#include <event2/dns.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    struct evhttp_uri *u = evhttp_uri_parse("http://abc:90");
    printf("scheme=%s\n", evhttp_uri_get_scheme(u));
    printf("host=%s\n", evhttp_uri_get_host(u));
    printf("port=%d\n", evhttp_uri_get_port(u));
    printf("path=%s\n", evhttp_uri_get_path(u));
    return 0;
}