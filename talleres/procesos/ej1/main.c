#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Total times the child process must be signaled before executing argv[1]
#define SIGNALS_MAX 5

// Number of times the child process was signaled
int signals_count = 0;

void child_sigurg_handler(int signum) {
    signals_count++;
    printf("ya va!\n");
}

void parent_sigint_handler(int signum) {
    /* Wait for child and exit */
    int status;
    waitpid(-1, &status, 0);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "Uso: %s comando [argumentos ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Set a new handler for SIGINT on the parent */
    struct sigaction new_sigint_action;
    new_sigint_action.sa_handler = parent_sigint_handler;
    sigaction(SIGINT, &new_sigint_action, NULL);

    pid_t child = fork();
    if (child != 0) {
        /* Parent process */
        for (int i = 0; i < SIGNALS_MAX; ++i) {
            sleep(1);
            printf("sup!\n");
            /* Send SIGURG signal to the child process */
            kill(child, SIGURG);
        }
        /* TODO: solve this in a better way */
        while(1);
    }
    else {
        /* Child process */
        /* Set a new handler for SIGURG on the child */
        struct sigaction new_sigurg_action;
        new_sigurg_action.sa_handler = child_sigurg_handler;
        sigaction(SIGURG, &new_sigurg_action, NULL);

        /* sleep until having been interrupted SIGNALS_MAX times */
        while (signals_count < SIGNALS_MAX);

        /* send SIGINT to the parent id */
        pid_t parent_id = getppid();
        kill(parent_id, SIGINT);

        /* Execute the indicated program with it's parameters */
        execvp(argv[1], argv+1);

        /* exit */
        exit(EXIT_SUCCESS);
    }
}
