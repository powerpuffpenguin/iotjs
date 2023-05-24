#include <tomcrypt.h>
#include <stdio.h>
#include <stdlib.h>
void puts_hex(const unsigned char *s, int n)
{
    static unsigned char *hextable = "0123456789abcdef";
    int j = 0;
    for (int i = 0; i < n; i++)
    {
        unsigned char v = s[i];
        putc(hextable[v >> 4], stdout);
        putc(hextable[v & 0x0f], stdout);
        j += 2;
    }
    putc('\n', stdout);
}
int main(int argc, char *argv[])
{
    hash_state md;
    if (sha512_desc.init(&md))
    {
        puts("sha512_init error");
        return 1;
    }
    if (sha512_desc.process(&md, "ok", 2))
    {
        puts("sha512_init error");
        return 1;
    }
    printf("%ld\n", sha512_desc.hashsize);
    unsigned char *out = malloc(sha512_desc.hashsize);
    if (!out)
    {
        puts("malloc error");
        return 1;
    }
    if (sha512_desc.done(&md, out))
    {
        puts("sha512_init error");
        return 1;
    }
    puts_hex(out, sha512_desc.hashsize);
    return 0;
}