#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char SHM_NAME[] = "/shared_memory";
const int SHM_SIZE = 4096;
const char SEM_WRITE[] = "/sem_write_2";
const char SEM_READ[] = "/sem_read_2";

int main(int argc, char** argv) {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    char *shared_memory = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    sem_t *sem_write = sem_open(SEM_WRITE, 0);
    sem_t *sem_read = sem_open(SEM_READ, 0);
    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem_read);

        char *buf = (char *)shared_memory;
        ssize_t len = strnlen(buf, SHM_SIZE);

        if (len == 0 || buf[0] == '\n') {
            sem_post(sem_write);
            break;
        }

        char reversed_buf[(len - 1)];
        for (ssize_t i = 0; i < (len - 1); i++) {
            reversed_buf[(len - 1) - i - 1] = buf[i];
        }

        reversed_buf[(len - 1)] = '\n';

        int32_t written = write(STDOUT_FILENO, reversed_buf, len);
        if (written != len) {
            const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        sem_post(sem_write);
    }

    munmap(shared_memory, SHM_SIZE);
    sem_close(sem_write);
    sem_close(sem_read);

    return 0;
}
