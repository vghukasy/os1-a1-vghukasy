#include "kernel/types.h"
#include "user/user.h"

static void
usage(void) 
{
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
    }

    char *s = argv[1];
    if (*s == 0) {
        usage();
    }

    int ticks = 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9') {
            usage();
        }

        int digit = *s - '0';
        // reject values that would overflow int
        if (ticks > (0x7fffffff - digit) / 10) {
            usage();
        }
        
        ticks = ticks * 10 + digit;
    }

    if (ticks > 0) {
        pause(ticks);
    }

    exit(0);
}