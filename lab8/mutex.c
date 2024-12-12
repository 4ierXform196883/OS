#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define ARRAY_SIZE 27
#define NUM_READERS 10

char shared_array[ARRAY_SIZE];
bool writer_finished = false;

pthread_mutex_t mutex;

void* writer_thread(void* arg) {
    for (int i = 0; i < ARRAY_SIZE - 1; i++) {
        pthread_mutex_lock(&mutex);
        shared_array[i] = 'A' + i;
        printf("[WRITER]: added %c\n", shared_array[i]);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }

    pthread_mutex_lock(&mutex);
    writer_finished = true;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* reader_thread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        printf("[READER %lu]: %s\n", pthread_self(), shared_array);

        if (writer_finished) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_mutex_unlock(&mutex);
        sleep(1);
    }

    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[NUM_READERS];

    for (int i = 0; i < ARRAY_SIZE - 1; i++) {
        shared_array[i] = '-';
    }
    shared_array[ARRAY_SIZE - 1] = '\0';

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("pthread_mutex_init() failed to initiate mutex");
        return 1;
    }

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
        perror("pthread_create() failed to create writer thread");
        return 1;
    }

    for (int i = 0; i < NUM_READERS; i++) {
        if (pthread_create(&readers[i], NULL, reader_thread, NULL) != 0) {
            perror("pthread_create() failed to create writer thread");
            return 1;
        }
    }

    // if req: 2-d arg <- void* status for end status
    if (pthread_join(writer, NULL) != 0) {
        perror("pthread_join() failed to join writer thread");
        return 1;
    }
    for (int i = 0; i < NUM_READERS; i++) {
        if (pthread_join(readers[i], NULL) != 0) {
            perror("pthread_join() failed to join reader thread");
            return 1;
        }
    }

    pthread_mutex_destroy(&mutex);

    return 0;
}
