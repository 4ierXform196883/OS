#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define MESSAGE_SIZE 32
#define FILE_NAME "shmem_file"

char* shmem_ptr = NULL;

void signal_handler(int signal) {
    if (shmem_ptr) {
        if (shmdt(shmem_ptr) == -1) {
            perror("shmdt() failed");
            exit(1);
        }
    }

    exit(0);
}

int main() {

    key_t key = ftok(FILE_NAME, 1);
    if (key == -1) {
        perror("ftok() failed");
        return -1;
    }

    int shmid = shmget(key, MESSAGE_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget() failed");
        return -1;
    }

    shmem_ptr = shmat(shmid, NULL, SHM_RDONLY);
    if (shmem_ptr == (void *)-1) {
        perror("shmat() failed");
        return -1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    while (1) {
        sleep(1);
        
        char time_str[MESSAGE_SIZE] = {0};
        char message[MESSAGE_SIZE] = {0};

        strcpy(message, shmem_ptr);

        time_t timestamp = time(NULL);
        struct tm* current_time = localtime(&timestamp);

        strftime(time_str, sizeof(time_str), "%H:%M:%S", current_time);

        printf("[RECEIVER] %s; pid: %d;\n[RECEIVED MESSAGE]: %s\n", time_str, getpid(), message);
    }

    return 0;
}