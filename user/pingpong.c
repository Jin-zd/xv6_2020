#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(void) {
    // double pipelines
    // p1[0]: parent write to child; p1[1]: parent read from child
    // p2[0]: child write to parent; p2[1]: child read from parent
    int p1[2], p2[2];
    char p_read, c_read;
    pipe(p1);
    pipe(p2);
    if (fork() == 0) {
        // in child, close p1 output and p2 input
        close(p1[1]);
        close(p2[0]);
        int flag = read(p1[0], &c_read, 1);
        int child_pid = getpid();
        if (flag == 1) {
            write(p2[1], &c_read, 1);
            printf("%d: received ping\n", child_pid);
            close(p1[0]);
            close(p2[1]);
            exit(0);
        }
    } else {
        // in parent, close p2 output and p1 input
        close(p1[0]);
        close(p2[1]);
        write(p1[1], "p", 1);
        int flag = read(p2[0], &p_read, 1);
        int parent_pid = getpid();
        wait(0);
        if (flag == 1) {
            printf("%d: received pong\n", parent_pid);
            close(p1[1]);
            close(p2[0]);
            exit(0);
        }
    }

    exit(0);
}