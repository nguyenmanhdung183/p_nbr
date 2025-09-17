#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define NUM_PROCESSES 256

void run(sem_t *sem, int* counter){
            sem_wait(sem);
            (*counter)++;
            sem_post(sem);
            return;
}

int main() {
    // Tạo biến counter trong vùng nhớ chia sẻ
    int *counter = mmap(NULL, sizeof(int),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS,
                        -1, 0);
    if (counter == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // Khởi tạo counter = 0
    *counter = 0;

    // Tạo semaphore trong vùng nhớ chia sẻ
    sem_t *sem = mmap(NULL, sizeof(sem_t),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS,
                      -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap (sem) failed");
        exit(1);
    }

    // Khởi tạo semaphore
    if (sem_init(sem, 1, 1) < 0) { // 1 = shared giữa các process
        perror("sem_init failed");
        exit(1);
    }

    // Tạo 256 tiến trình
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Tiến trình con

            run(sem, counter);

            // Thoát tiến trình con
            exit(0);
        }
    }

    // Đợi tất cả tiến trình con kết thúc
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    // In kết quả cuối cùng
    printf("Final counter value: %d\n", *counter);

    // Hủy semaphore và giải phóng vùng nhớ
    sem_destroy(sem);
    munmap(counter, sizeof(int));
    munmap(sem, sizeof(sem_t));

    return 0;
}
