#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>

int main(int argc, char* argv[]) {
    pid_t child;

    if (argc <= 1) {
        fprintf(stderr, "Uso: %s commando [argumentos ...]\n", argv[0]);
        exit(1);
    }
    child = fork();
    if (child == -1) {
        perror("ERROR fork");
        return 1;
    }
    if (child == 0) {
        /* Child process */
        int trace_ret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv+1);
        perror("ERROR child exec(...)");
        exit(1);
    }
    else {
        int status;
        while(1) {
            /* Parent process */
            if (wait(&status) < 0) {
                perror("waitpid");
                break;
            }
            if (WIFEXITED(status) || WIFSIGNALED(status))
                break;
            int syscall_enter = ptrace(PTRACE_PEEKUSER, child, 8*RAX, NULL);
            if (syscall_enter == -ENOSYS) {
                /* Child process entered a syscall */
                int sysno = ptrace(PTRACE_PEEKUSER, child, 8*ORIG_RAX, NULL);
                if (sysno == SYS_kill) {
                    /* Child executed a kill */
                    printf("Se ha hecho justicia!\n");
                    ptrace(PTRACE_KILL, child, NULL, NULL);
                }
            }
            /* Continue until next syscall */
            ptrace(PTRACE_SYSCALL, child, 0, 0);
        }
        ptrace(PTRACE_DETACH, child, NULL, NULL);
    }
    return 0;
}
