#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

const int LENGTH_FILTER = 10;
const int MAX_BUFFER_SIZE = 4096;
const int MAX_FILE_NAME_SIZE = 1024;
const int MAX_MESSAGE_SIZE = 1024;

static char progpath[] = ".";
static char CLIENT_PROGRAM_NAME_1[] = "child1";
static char CLIENT_PROGRAM_NAME_2[] = "child2";

int main(int argc, char** argv) {
    if (argc != 1) {
        char msg[MAX_MESSAGE_SIZE];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    char buf[MAX_BUFFER_SIZE];
    ssize_t bytes;

    // char file_name_1[] = "file1.txt";
    // char file_name_2[] = "file2.txt";

    // вводим первое имя файла
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

    // открываем первый файл
	int32_t file1 = open(file_name_1, O_WRONLY, 0600);
	if (file1 == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    // вводим второе имя файла
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

    // открываем второй файл
	int32_t file2 = open(file_name_2, O_WRONLY, 0600);
	if (file2 == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    // создаём pipes
    int pipe1[2];
    int pipe2[2];

    if ((pipe(pipe1) == -1) || (pipe(pipe2) == -1)) {
        const char msg[] = "error: failed to create pipes\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // первый child
    const pid_t child1 = fork();

    if (child1 == -1) {
        const char msg[] = "error: failed to spawn first child\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);

    } else if (child1 == 0) {
        close(pipe1[1]); // закрываем запись
        dup2(pipe1[0], STDIN_FILENO); // меняем stdin на чтение из pipe
        dup2(file1, STDOUT_FILENO); // меняем stdout на запись в file

        char path[MAX_FILE_NAME_SIZE];
        int tmp = snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME_1);
        char *const args[] = {CLIENT_PROGRAM_NAME_1, NULL};

        int32_t status = execv(path, args);
        if (status == -1) {
            const char msg[] = "error: failed to exec into new exectuable image\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]);
    }

    // второй child
    const pid_t child2 = fork();

    if (child2 == -1) {
        const char msg[] = "error: failed to spawn second child\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);

    } else if (child2 == 0) {
        close(pipe2[1]); // закрываем запись
        dup2(pipe2[0], STDIN_FILENO); // меняем stdin на чтение из pipe
        dup2(file2, STDOUT_FILENO); // меняем stdout на запись в file

        char path[MAX_FILE_NAME_SIZE];
        snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME_2);
        char* const args[] = {CLIENT_PROGRAM_NAME_2, NULL};

        int32_t status = execv(path, args);
        if (status == -1) {
            const char msg[] = "error: failed to exec into new exectuable image\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        close(pipe2[0]);
    }

    // parent

    close(pipe1[0]); // закрываем чтение
    close(pipe2[0]); // закрываем чтение

    {
        const char msg[] = "Start typing lines of text. Press 'Ctrl-D' or 'Enter' with no input to exit\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            write(pipe1[STDOUT_FILENO], buf, bytes);
            write(pipe2[STDOUT_FILENO], buf, bytes);
            break;
        }

        if (bytes > LENGTH_FILTER) {
            write(pipe2[STDOUT_FILENO], buf, bytes);
        } else {
            write(pipe1[STDOUT_FILENO], buf, bytes);
        }
    }

    int child_status;

    waitpid(child1, &child_status, 0);
    waitpid(child2, &child_status, 0);

    if (child_status != EXIT_SUCCESS) {
        const char msg[] = "error: a child exited with error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(child_status);
    }

    close(pipe1[STDOUT_FILENO]); // закрываем запись
    close(pipe2[STDOUT_FILENO]); // закрываем запись

    close(file1);
    close(file2);

    return 0;
}
