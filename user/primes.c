#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX_NUMS 280
// remove all the nums which is a multiple of k and print k
void remove_nums(int* nums, int k) {
    for (int i = 0; i < MAX_NUMS - 1; i++) {
        if (nums[i] == k) {
            nums[i] = 0;
            printf("prime %d\n", k);
        } else if (nums[i] != 0 && nums[i] % k == 0) {
            nums[i] = 0;
        }
    } 
}
// if all of the nums is 0, finish
int is_finish(int* nums) {
    for (int i = 0; i < MAX_NUMS - 1; i++) {
        if (nums[i] != 0) {
            return 0;
        }
    }
    return 1;
}
int main(void) {
    int nums[MAX_NUMS - 1];
    // initial
    for (int i = 0; i < MAX_NUMS - 1; i ++) {
        nums[i] = i + 2;
    }
    while(1) {
        // one statment is enough
        int p[2];
        pipe(p);
        
        if (fork() == 0) {
            // child, read from parent
            read(p[0], nums, (MAX_NUMS - 1));
            close(p[0]);
            close(p[1]);
            // remove nums which is not a prime
            for (int i = 0; i < MAX_NUMS - 1; i++) {
                if (nums[i] != 0) {
                    remove_nums(nums, nums[i]);
                    break;
                }
            }
            // if finish, exit
            if (is_finish(nums)) {
                exit(0);
            }
        } else {
            // parent, convert nums to child
            close(p[0]);
            write(p[1], nums, (MAX_NUMS - 1));
            close(p[1]);
            // wait for child done
            wait(0);
            exit(0);
        }
    }
}