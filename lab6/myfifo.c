#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

int main()
{
    pid_t pid;
    char message[256];

    if (mkfifo("FIFO", 0666) == -1) {
        perror("mkfifo() failed to create FIFO");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork() failed to initiate child process");
        exit(1);
    }

    if (pid > 0) {
        time_t parent_time = time(NULL);

        printf("[PARENT] my pid: %d; current time: %s\n", getpid(), ctime(&parent_time));
        sprintf(message, "[PARENT] my pid: %d; current time: %s", getpid(), ctime(&parent_time));

        sleep(5);

        int fd = open("FIFO", O_WRONLY);
        if (fd == -1) {
            perror("failed to open FIFO in write mode");
            exit(1);
        }

        write(fd, message, sizeof(message));

        close(fd);
        unlink("FIFO");
    }
    else {
        int fd = open("FIFO", O_RDONLY);
        if (fd == -1) {
            perror("failed to open FIFO in read only mode");
            exit(1);
        }

        read(fd, message, sizeof(message));

        time_t child_time = time(NULL);
        printf("[CHILD] my pid: %d; current time: %s\n", getpid(), ctime(&child_time));
        printf("[RECEIVED MESSAGE]: %s\n", message);

        close(fd);
    }

    return 0;
}
