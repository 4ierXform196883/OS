#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/sem.h>

#define MESSAGE_SIZE 32
#define FILE_NAME "shmem_file"

char* shmem_ptr = NULL;
int shmem_id = -1;
int sem_id = -1;
struct sembuf sem_lock = { 0, -1, 0 }, sem_open = { 0, 1, 0 };

void signal_handler(int signal) {
    if (shmem_ptr != NULL) {
        if (shmdt(shmem_ptr) != 0) {
            perror("shmdt() failed");
            exit(1);
        }
    }

    if (shmem_id != -1) {
        if (shmctl(shmem_id, IPC_RMID, NULL) != 0) {
            perror("shmctl() failed");
            exit(1);
        }
    }

    if (sem_id != -1) {
        if (semctl(sem_id, 0, IPC_RMID) != 0) {
            perror("semctl() failed");
            exit(1);
        }
    }

    if (remove(FILE_NAME) == -1) {
        perror("remove() failed");
        exit(1);
    }

    exit(0);
}

int main() {
    int fd = open(FILE_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        if (access(FILE_NAME, F_OK) == 0) {
            printf("Program is already running in another process.\nPlease terminate it before launching again.\n");
            return 0;
        }
        perror("open() failed");
        return 1;
    }
    close(fd);

    key_t key = ftok(FILE_NAME, 1);
    if (key == -1) {
        perror("ftok() failed");
        return 1;
    }

    shmem_id = shmget(key, MESSAGE_SIZE, 0666 | IPC_CREAT);
    if (shmem_id == -1) {
        perror("shmget() failed");
        return 1;
    }

    sem_id = semget(key, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget() failed");
        return 1;
    }

    semop(sem_id, &sem_open, 1);

    shmem_ptr = shmat(shmem_id, NULL, 0);
    if (shmem_ptr == (void*)-1) {
        perror("shmat() failed");
        return 1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    while (1) {
        char time_str[MESSAGE_SIZE] = { 0 };
        char message[MESSAGE_SIZE] = { 0 };

        time_t timestamp = time(NULL);
        struct tm* current_time = localtime(&timestamp);

        strftime(time_str, MESSAGE_SIZE, "%H:%M:%S; pid: %%d\n", current_time);
        sprintf(message, time_str, getpid());

        semop(sem_id, &sem_lock, 1);
        strcpy(shmem_ptr, message);
        sleep(1);
        semop(sem_id, &sem_open, 1);
    }

    return 0;
}
