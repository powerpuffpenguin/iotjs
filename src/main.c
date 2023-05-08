#include <stdio.h>

#include "vm.h"
#include "duktape.h"

int main(int argc, char *argv[])
{
    char *filename;
    if (argc >= 2)
    {
        filename = argv[1];
    }
    else
    {
        filename = "main.js";
    }

    int errno = 0;
    vm_t *vm = vm_new(&errno);
    if (errno)
    {
        puts(vm_error(errno));
        return errno;
    }

    printf("load main: %s\n", filename);
    duk_context *ctx = vm->ctx;
    errno = vm_main(vm, filename);
    if (errno)
    {
        printf("eval failed: %s\n", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        return errno;
    }
    duk_pop(ctx);

    return 0;
}