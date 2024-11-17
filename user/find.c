#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#define MAX_PATH 512
void find(char *current_dir, const char *file_name) {
    char buf[MAX_PATH];
    int fd;
    struct dirent de;
    struct stat st;
    // try to open dir
    if ((fd = open(current_dir, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", current_dir);
        return;
    }
    // try to get fd's stat
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", current_dir);
        close(fd);
        return;
    }
    // when fd isn't dir, return
    if (st.type != T_DIR) {
        close(fd);
        return;
    }
    // judge whether the dir's len is over
    int dir_len = strlen(current_dir);
    if (dir_len + 1 + DIRSIZ + 1 > MAX_PATH) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return;
    }
    // construct "dir/"
    strcpy(buf, current_dir);
    char *p = buf + dir_len;
    *p++ = '/';
    // use a loop to get all the files under the dir
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        // when dir is "." or "..", don't do recursion
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        // get the file's path
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        // try to get the file's stat
        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        // if the file is a file, compare and print the result, 
        // if the file is a dir, do recursion.
        if (st.type == T_FILE && strcmp(de.name, file_name) == 0) {
            printf("%s\n", buf);
        } else if (st.type == T_DIR) {
            find(buf, file_name);
        }
    }
    close(fd);
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}