#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(1, "sleep: sleep command needs arguments!\n", 37);
        exit(1);
    } else {
        int sleep_num = atoi(argv[1]);
        if (sleep_num < 0) {
            write(1, "Error: sleep command needs positive arguments!\n", 46);
            exit(1);
        }
        sleep(sleep_num);
    }
    exit(0);
}
