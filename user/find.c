#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

static char *name_pattern;
static char type_filter;

static int
matches_pattern(char *pattern, char *text)
{
    if (*pattern == 0) {
        return *text == 0;
    }

    if (*pattern == '*') {
        return matches_pattern(pattern + 1, text) || (*text && matches_pattern(pattern, text + 1));
    }

    return *pattern == *text && matches_pattern(pattern + 1, text + 1);
}

static int
search_directory(char *directory_path)
{
    char entry_path[MAXPATH];
    char *entry_name;
    struct dirent entry;
    struct stat directory_stat;
    struct stat entry_stat;

    int directory_fd = open(directory_path, 0);
    if (directory_fd < 0) {
        return -1;
    }

    if (fstat(directory_fd, &directory_stat) < 0 || directory_stat.type != T_DIR || strlen(directory_path) + 1 + DIRSIZ >= sizeof entry_path) {
        close(directory_fd);
        return 0;
    }

    strcpy(entry_path, directory_path);
    entry_name = entry_path + strlen(entry_path);
    if (entry_name[-1] != '/') {
        *entry_name++ = '/';
    }
    
    while (read(directory_fd, &entry, sizeof(entry)) == sizeof(entry)){
        if (entry.inum == 0) {
            continue;
        }
        
        memmove(entry_name, entry.name, DIRSIZ);
        entry_name[DIRSIZ] = 0;
        
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }
        
        if (stat(entry_path, &entry_stat) < 0) {
            continue;
        }

        int is_directory = entry_stat.type == T_DIR;
        int type_matches = type_filter == 0 || (type_filter == 'd') == is_directory;

        if (type_matches && matches_pattern(name_pattern, entry_name)) {
            printf("%s\n", entry_path);
        } if (is_directory) {
            search_directory(entry_path);
        }
    }

    close(directory_fd);
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc == 5 && strcmp(argv[3], "-type") == 0 && (strcmp(argv[4], "f") == 0 || strcmp(argv[4], "d") == 0)) {
        type_filter = argv[4][0];
    } else if(argc != 3) {
        fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
        exit(1);
    }
    
    char *start_directory = argv[1];
    name_pattern = argv[2];
    
    if (search_directory(start_directory) < 0){
        fprintf(2, "find: cannot open %s\n", start_directory);
        exit(1);
    }
    
    exit(0);
}
