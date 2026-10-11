#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
        exit(1);
    }

    if (trace(atoi(argv[1])) < 0) {
        fprintf(2, "trace: trace failed\n");
        exit(1);
    }

    char **command_argv = &argv[2];
    exec(command_argv[0], command_argv);
    fprintf(2, "trace: exec %s failed\n", command_argv[0]);
    exit(1);
}