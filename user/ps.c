#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

static char *states[] = {
    "unused", 
    "used", 
    "sleeping", 
    "runnable", 
    "running", 
    "zombie"
};

static struct uproc processes[NPROC];

int
main(void)
{
    int process_count = getprocs(processes, NPROC);
    if (process_count < 0) {
        fprintf(2, "ps: getprocs failed\n");
        exit(1);
    }

    printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");
    for (int i = 0; i < process_count; i++) {
        struct uproc *process = &processes[i];
        int known_state = process->state >= 0 && process->state < sizeof(states) / sizeof( states[0]);

        printf("%d\t%d\t%s\t%lu\t%s\n", process->pid, process->ppid, known_state ? states[process->state] : "?", process->sz, process->name);
    }
    exit(0);
}