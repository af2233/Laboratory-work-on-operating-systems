#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

const int MAX_BUFFER_SIZE = 4096;

int main(int argc, char** argv) {
	char buf[MAX_BUFFER_SIZE];
	ssize_t bytes;

    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            break;
        }

        ssize_t len = bytes - 1;

        char reversed_buf[len];

        for(ssize_t i = 0; i < len; i++) {
            reversed_buf[len - i - 1] = buf[i];
        }

        reversed_buf[len] = '\n';

        int32_t written = write(STDOUT_FILENO, reversed_buf, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
