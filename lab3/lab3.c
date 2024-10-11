#define _POSIX_C_SOURCE 199309
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

void parent1_exit_handler() {
    printf("[PARENT1] process termination; pid: %d\n", getpid());
}

void child1_parent2_exit_handler() {
    printf("[CHILD1 = PARENT2] process termination; pid: %d\n", getpid());
}

void child2_exit_handler() {
    printf("[CHILD2] process termination; pid: %d\n", getpid());
}

void sigint_handler(int signum) {
    printf("Process %d received SIGINT; signal number: %d\n", getpid(), signum);
}

void sigterm_handler(int signum, siginfo_t *info, void *context) {
    if (info != NULL) {
        printf("Process %d recieved SIGTERM; signal %d from process %d\n", getpid(), signum, info->si_pid);
    }
    else {
        printf("Process %d recieved SIGTERM; signal %d\n", getpid(), signum);
    }
}

int main(int argc, char** argv) {
    printf("[MAIN PROCESS] my pid: %d\n", getpid());
    pid_t pid;

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal() failed to set up sigint handler\n");
        exit(errno);
    }

    struct sigaction custom_sigaction;
    custom_sigaction.sa_sigaction = sigterm_handler;
    custom_sigaction.sa_flags = SA_SIGINFO;
    if (sigaction(SIGTERM, &custom_sigaction, NULL) == -1) {
        perror("sigaction() failed to set up sigterm handler\n");
        exit(errno);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork() failed to initiate child process\n");
        exit(errno);
    }
    if (pid == 0) {
        printf("[CHILD1 = PARENT2] my pid: %d; my parent's pid: %d\n", getpid(), getppid());
        pid_t pid1;
        sleep(5);

        pid1 = fork();
        if (pid1 == -1) {
            perror("fork() failed to initiate child process\n");
            exit(errno);
        }
        if (pid1 == 0) {
            if (atexit(child2_exit_handler) == -1) {
                perror("atexit() failed to set up exit handler\n");
                exit(errno);
            }
            printf("[CHILD2] my pid: %d; my parent's pid: %d\n", getpid(), getppid());
            sleep(2);
        }
        else {
            if (atexit(child1_parent2_exit_handler) == -1) {
                perror("atexit() failed to set up exit handler\n");
                exit(errno);
            }
            printf("[PARENT2] my pid: %d; my child's pid: %d; my parent's pid: %d\n", getpid(), pid1, getppid());
            sleep(1);
        }

    }
    else {
        if (atexit(parent1_exit_handler) == -1) {
            perror("atexit() failed to set up exit handler\n");
            exit(errno);
        }
        printf("[PARENT1] my pid: %d; my child's pid: %d; my parent's pid: %d\n", getpid(), pid, getppid());
        int status;
        pid = wait(&status);
        printf("[PARENT1] has waited for termination of process %d, which exited with status %d\n", pid, status);
    }
    return 0;
}