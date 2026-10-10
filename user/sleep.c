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
    if (argc != 2 || argv[1][0] == '\0') {
        usage();
    }

    int n = 0;
    for (char *string = argv[1]; *string != 0; string++) {
        if (*string < '0' || *string > '9') {
            usage();
        }
        n = n * 10 + (*string - '0');
    }

    pause(n);
    exit(0);
}