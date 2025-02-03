#include <fcntl.h>     // shm_open, O_*
#include <semaphore.h> // семафор
#include <stdio.h>     // snprintf
#include <stdlib.h>
#include <string.h>    // memset, strncopy
#include <sys/mman.h>  // mmap, PROT_*, MAP_*
#include <sys/wait.h>  // waitpid
#include <unistd.h>    // fork, write, read

const int LENGTH_FILTER = 10;
const int MAX_FILE_NAME_SIZE = 1024;

const char SHM_NAME[] = "/shared_memory";
const int SHM_SIZE = 4096;

const char SEM_WRITE_1[] = "/sem_write_1";
const char SEM_READ_1[] = "/sem_read_1";

const char SEM_WRITE_2[] = "/sem_write_2";
const char SEM_READ_2[] = "/sem_read_2";

static char progpath[] = ".";
static char CLIENT_PROGRAM_NAME_1[] = "child1";
static char CLIENT_PROGRAM_NAME_2[] = "child2";

int main(int argc, char** argv) {
    if (argc != 1) {
        const char msg[] = "usage: ./executable\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    // Создаем общую память
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    // Отображаем память в адресное пространство
    char *shared_memory = mmap(NULL, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Создаем семафоры
    sem_t *sem_write_1 = sem_open(SEM_WRITE_1, O_CREAT, 0666, 1); // Разрешаем запись
    sem_t *sem_read_1 = sem_open(SEM_READ_1, O_CREAT, 0666, 0);  // Блокируем чтение
    if (sem_write_1 == SEM_FAILED || sem_read_1 == SEM_FAILED) {
        exit(EXIT_FAILURE);
    }

    sem_t *sem_write_2 = sem_open(SEM_WRITE_2, O_CREAT, 0666, 1); // Разрешаем запись
    sem_t *sem_read_2 = sem_open(SEM_READ_2, O_CREAT, 0666, 0);  // Блокируем чтение
    if (sem_write_2 == SEM_FAILED || sem_read_2 == SEM_FAILED) {
        exit(EXIT_FAILURE);
    }

    // Вводим имена файлов
    // char file_name_1[] = "file1.txt";
    // char file_name_2[] = "file2.txt";

    char buf[SHM_SIZE];
    ssize_t bytes;

    {
        const char msg[] = "Enter first file name:\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    bytes = read(STDIN_FILENO, buf, sizeof(buf));
    if (bytes < 0) {
        const char msg[] = "error: failed to read from stdin\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    char file_name_1[MAX_FILE_NAME_SIZE];
    for (int i = 0; i < (bytes - 1); i++) {
        file_name_1[i] = buf[i];
    }

    {
        const char msg[] = "Enter second file name:\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    bytes = read(STDIN_FILENO, buf, sizeof(buf));
    if (bytes < 0) {
        const char msg[] = "error: failed to read from stdin\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    char file_name_2[MAX_FILE_NAME_SIZE];
    for (int i = 0; i < (bytes - 1); i++) {
        file_name_2[i] = buf[i];
    }

    // Открываем файлы
	int32_t file1 = open(file_name_1, O_WRONLY, 0600);
	if (file1 == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int32_t file2 = open(file_name_2, O_WRONLY, 0600);
	if (file2 == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    // Создаем дочерние процессы
    const pid_t child1 = fork();

    if (child1 == -1) {
        const char msg[] = "error: failed to spawn first child\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);

    } else if (child1 == 0) {
        dup2(file1, STDOUT_FILENO); // Перенаправляем вывод в файл

        char path[MAX_FILE_NAME_SIZE];
        snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME_1);
        char *const args[] = {CLIENT_PROGRAM_NAME_1, NULL};

        int32_t status = execv(path, args);
        if (status == -1) {
            const char msg[] = "error: failed to exec child process\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    const pid_t child2 = fork();

    if (child2 == -1) {
        const char msg[] = "error: failed to spawn first child\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);

    } else if (child2 == 0) {
        dup2(file2, STDOUT_FILENO); // Перенаправляем вывод в файл

        char path[MAX_FILE_NAME_SIZE];
        snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME_2);
        char *const args[] = {CLIENT_PROGRAM_NAME_2, NULL};

        int32_t status = execv(path, args);
        if (status == -1) {
            const char msg[] = "error: failed to exec child process\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    // Родительский процесс

    const char msg[] = "Start typing lines of text. Press 'Enter' with no input to exit\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
            const char err[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, err, sizeof(err) - 1);
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            sem_wait(sem_write_2);
            memset(shared_memory, 0, SHM_SIZE);
            strncpy((char *)shared_memory, buf, bytes);
            sem_post(sem_read_2);

            sem_wait(sem_write_1);
            memset(shared_memory, 0, SHM_SIZE);
            strncpy((char *)shared_memory, buf, bytes);
            sem_post(sem_read_1);

            break;
        }

        if (bytes > LENGTH_FILTER) {
            sem_wait(sem_write_2);
            memset(shared_memory, 0, SHM_SIZE);
            strncpy((char *)shared_memory, buf, bytes);
            sem_post(sem_read_2);
        } else {
            sem_wait(sem_write_1);
            memset(shared_memory, 0, SHM_SIZE);
            strncpy((char *)shared_memory, buf, bytes);
            sem_post(sem_read_1);
        }
    }

    int child_status;
    waitpid(child1, &child_status, 0);
    waitpid(child2, &child_status, 0);

    if (child_status != EXIT_SUCCESS) {
        const char err[] = "error: a child exited with error\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(child_status);
    }

    // Освобождаем ресурсы
    munmap(shared_memory, SHM_SIZE);
    shm_unlink(SHM_NAME);

    sem_close(sem_write_1);
    sem_close(sem_read_1);
    sem_unlink(SEM_WRITE_1);
    sem_unlink(SEM_READ_1);

    sem_close(sem_write_2);
    sem_close(sem_read_2);
    sem_unlink(SEM_WRITE_2);
    sem_unlink(SEM_READ_2);

    close(file1);
    close(file2);

    return 0;
}
