#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_WORD_LENGTH 128
#define END_OF_INPUT (-1)

static char word_storage[MAXARG][MAX_WORD_LENGTH];

void
usage(void)
{
    fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
    exit(1);
}

int
parse_positive_int(char *text)
{
    int value = 0;

    if (*text == 0) {
        usage();
    }

    for (; *text; text++) {
        if (*text < '0' || *text > '9') {
            usage();
        }

        if (value < MAXARG) {
            value = value * 10 + (*text - '0');
        }
    }
    
    if (value < 1) {
        usage();
    }
    
    return value;
}

int
read_word(char *word)
{
    int length = 0;
    char character;
    
    while (read(0, &character, 1) == 1) {
        if (character == ' ' || character == '\t' || character == '\n') {
            word[length] = 0;
            return character;
        }

        if (length == MAX_WORD_LENGTH - 1) {
            fprintf(2, "xargs: word too long\n");
            exit(1);
        }

        word[length++] = character;
    }

    word[length] = 0;
    return END_OF_INPUT;
}

int
run_command(char *command_argv[])
{
    int child_pid = fork();
    if (child_pid < 0) {
        fprintf(2, "xargs: fork failed\n");
        exit(1);
    }
    
    if (child_pid == 0) {
        close(0);
        exec(command_argv[0], command_argv);
        fprintf(2, "xargs: exec %s failed\n", command_argv[0]);
        exit(1);
    }

    int exit_status = 1;
    wait(&exit_status);
    return exit_status == 0;
}

int
main(int argc, char *argv[])
{
    int words_per_run = 0;
    int command_start = 1;
    
    if (argc >= 2 && strcmp(argv[1], "-n") == 0) {
        if (argc < 4) {
            usage();
        }
        
        words_per_run = parse_positive_int(argv[2]);
        command_start = 3;
    }

    if (command_start >= argc) {
        usage();
    }

    char *command_argv[MAXARG + 1];
    int fixed_count = argc - command_start;

    int max_total = MAXARG - 1;
    if (fixed_count >= max_total) {
        fprintf(2, "xargs: too many arguments\n");
        exit(1);
    }
    int max_extra = max_total - fixed_count;

    if (words_per_run > max_extra) {
        words_per_run = max_extra;
    }

    for (int i = 0; i < fixed_count; i++) {
        command_argv[i] = argv[command_start + i];
    }

    int extra_count = 0;
    int all_runs_succeeded = 1;
    int terminator;

    do {
        char *word = word_storage[extra_count];
        terminator = read_word(word);

        if (word[0] != 0) {
            if (extra_count >= max_extra) {
                fprintf(2, "xargs: too many arguments\n");
                exit(1);
            }
            command_argv[fixed_count + extra_count] = word;
            extra_count++;
        }
        
        int batch_full = extra_count == words_per_run;
        int line_done = words_per_run == 0 && terminator == '\n';
        
        if (extra_count > 0 && (batch_full || line_done || terminator == END_OF_INPUT)) {
            command_argv[fixed_count + extra_count] = 0;
            
            if (!run_command(command_argv)) {
                all_runs_succeeded = 0;
            }

            extra_count = 0;
        }
    } while (terminator != END_OF_INPUT);

    exit(all_runs_succeeded ? 0 : 1);
}