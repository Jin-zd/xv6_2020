#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#define MAX_LINE 128


int main(int argc, char* argv[]) {
    char buf;
    char* new_argv[MAXARG];
    // get xargs's command argv
    for (int i = 1; i < argc; i++) {
        new_argv[i - 1] = argv[i];
    }
    char line[MAX_LINE];
    int i = 0;
    int flag;
    int next_line;
    int p_id = -1;
    do {
        // read char from standard input
        flag = read(0, &buf, 1);
        if (flag > 0) {
            if (strcmp(&buf, "\n") != 0) {
                line[i] = buf;
                i++;
            } else {
                next_line = 1;
            }
        }
        // when it will be next line, fork
        if (next_line) {
            next_line = 0;
            p_id = fork();
        }
        if (p_id == 0) {
            // in child, exec
            line[i + 1] = '\0';
            new_argv[argc - 1] = line;
            exec(argv[1], new_argv);
            exit(0);
        } else if (p_id > 0) {
            // wait for child and update var
            wait(0);
            i = 0;
            p_id = -1;
        }
    } while (flag != 0);
    
    exit(0);
}