#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

int main()
{
    pid_t pid;
    char message[256];
    int pipe_ids[2];

    if (pipe(pipe_ids) == -1) {
        perror("pipe() failed to create PIPE");
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

        close(pipe_ids[0]);

        sleep(5);

        write(pipe_ids[1], message, sizeof(message));

        close(pipe_ids[1]);
    }
    else {
        close(pipe_ids[1]);

        read(pipe_ids[0], message, sizeof(message));
        
        close(pipe_ids[0]);

        time_t child_time = time(NULL);
        printf("[CHILD] my pid: %d; current time: %s\n", getpid(), ctime(&child_time));
        printf("[RECEIVED MESSAGE]: %s\n", message);
    }

    return 0;
}
