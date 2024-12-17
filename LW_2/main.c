#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>


int MAX_THREADS;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int active_threads = 0;

typedef struct {
    double **matrix;
    int size;
    int column;
    double result;
} thread_data;


void free_matrix(double** matrix, int size) {
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }
    free(matrix);
}


double** malloc_matrix(int size) {
    double** matrix = (double**)malloc(size * sizeof(double*));
    for (int i = 0; i < size; i++) {
        matrix[i] = (double*)malloc(size * sizeof(double));
    }
    return matrix;
}


double** read_matrix(int* size) {
    write(STDOUT_FILENO, "Enter the matrix size: ", 24);

    char buf[4096];
    ssize_t bytes;

    bytes = read(STDIN_FILENO, buf, sizeof(buf));
    *size = atoi(buf);

    double** matrix = malloc_matrix(*size);

    write(STDOUT_FILENO, "Enter the matrix elements one by one.\n", 38);

    for (int i = 0; i < *size; i++) {
        for (int j = 0; j < *size; j++) {
            char msg[1024];
            uint32_t len = snprintf(msg, sizeof(msg), "Element [%d][%d]: ", i, j);
            write(STDOUT_FILENO, msg, len);

            bytes = read(STDIN_FILENO, buf, sizeof(buf));
            matrix[i][j] = atof(buf);
        }
    }
    return matrix;
}


void* thread_func(void* arg);


double determinant(double** matrix, int size) {
    if (size == 1) {
        return matrix[0][0];
    }
    if (size == 2) {
        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    }

    double det = 0;

    pthread_t threads[MAX_THREADS];
    thread_data args[MAX_THREADS];

    for (int col = 0; col < size; col++) {

        // Минор
        double** minor = malloc_matrix(size - 1);

        for (int i = 1; i < size; i++) {
            int minor_col = 0;
            for (int j = 0; j < size; j++) {
                if (j != col) {
                    minor[i - 1][minor_col] = matrix[i][j];
                    minor_col++;
                }
            }
        }

        // Поток
        args[col].matrix = minor;
        args[col].size = size - 1;
        args[col].column = col;
        args[col].result = 0;

        pthread_mutex_lock(&mutex);
        while (active_threads >= MAX_THREADS) {
            pthread_cond_wait(&cond, &mutex);
        }
        active_threads++;
        pthread_mutex_unlock(&mutex);

        pthread_create(&threads[col], NULL, thread_func, &args[col]);
        
    }

    for (int col = 0; col < size; col++) {
        pthread_join(threads[col], NULL);
        det += (col % 2 == 0 ? 1 : -1) * matrix[0][col] * args[col].result;
        free_matrix(args[col].matrix, size - 1);
    }

    return det;
}


void* thread_func(void* arg) {
    thread_data* data = (thread_data*)arg;
    data->result = determinant(data->matrix, data->size);

    pthread_mutex_lock(&mutex);
    active_threads--;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return NULL;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: ./program MAX_THREADS\n", 30);
        exit(EXIT_SUCCESS);
    }

    MAX_THREADS = atoi(argv[1]); // Максимальное количество потоков
    if (MAX_THREADS <= 0) {
        write(STDOUT_FILENO, "MAX_THREADS must be a positive integer.\n", 40);
        exit(EXIT_SUCCESS);
    }

    int matrix_size;
    double** matrix = read_matrix(&matrix_size);

    // Определитель
    double determinant_value = determinant(matrix, matrix_size);

    free_matrix(matrix, matrix_size);

    char msg[1024];
    uint32_t len = snprintf(msg, sizeof(msg) - 1, "Determinant: %.2lf\n", determinant_value);
    write(STDERR_FILENO, msg, len);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
