#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>  /* pid_t */
#include <sys/wait.h>   /* waitpid */
#include <unistd.h>     /* exit, fork */

/**
 * Notas: en todas las llamadas a dup2(), redirijo así:
 * La entrada estandar se redirige/reemplaza por el mio de lectura (salvo el primero)
 * La salida estandar se redirige/reemplaza por el de escritura del siguiente (salvo el ultimo)
 */


int run(char *program_name[], char **program_argv[], unsigned int count) {
    int the_pipes[count][2];
    for (int i = 0; i < count; ++i) {
        if (pipe(the_pipes[i])) {
            perror("pipes");
            exit(1);
        }
    }

    for (int i = 0; i < count; ++i) {
        pid_t current_child = fork();
        int my_order = i;
        if (current_child == 0) {
            /* Alguno de los procesos hijos */
            if (my_order == 0) {
                /* Primer hijo */

                /* Cierro los pipes sin usar */
                for (int i = 0; i < count; ++i) {
                    close(the_pipes[i][0]);
                    if (i != my_order + 1)
                        close(the_pipes[i][1]);
                }

                /* Duplico los file descriptors y ejecuto */
                dup2(the_pipes[my_order+1][1], STDOUT_FILENO);
                execvp(program_name[my_order], program_argv[my_order]);
            }
            else if (my_order < count -1 ) {
                /* Alguno del medio */

                /* Cierro los pipes sin usar */
                for (int i = 0; i < count; ++i) {
                    if (i != my_order)
                        close(the_pipes[i][0]);
                    if (i != my_order + 1)
                        close(the_pipes[i][1]);
                }

                /* Duplico los file descriptors y ejecuto */
                dup2(the_pipes[my_order][0], STDIN_FILENO);
                dup2(the_pipes[my_order+1][1], STDOUT_FILENO);
                execvp(program_name[my_order], program_argv[my_order]);
            }
            else {
                /* Ultimo hijo */

                /* Cierro los pipes sin usar */
                for (int i = 0; i < count; ++i) {
                    if (i != my_order)
                        close(the_pipes[i][0]);
                    close(the_pipes[i][1]);
                }

                /* Duplico los file descriptors y ejecuto */
                dup2(the_pipes[my_order][0], STDIN_FILENO);
                execvp(program_name[my_order], program_argv[my_order]);
            }
        }
    }

    /* Cierro los pipes */
    for (int i = 0; i < count; ++i) {
        close(the_pipes[i][0]);
        close(the_pipes[i][1]);
    }

    /* El padre espera a que terminen todos los hijos y sale */
    int ignored_status;
    for (int i = 0; i < count; ++i)
        wait(&ignored_status);

    return 0;
}

int main(int argc, char* argv[]) {

    /* Parsing de "ls -al | wc | awk '{ print $2 }'" */
    char *program_name[] = {
        "/bin/ls",
        "/usr/bin/wc",
        "/usr/bin/awk",
    };

    char *ls_argv[] = {"ls", "-al", NULL};
    char *wc_argv[] = {"wc", NULL};
    char *awk_argv[] = {"awk", "{ print $2 }", NULL};

    char **program_argv[] = {
        ls_argv,
        wc_argv,
        awk_argv,
    };

    unsigned int count = 3;

    int status = run(program_name, program_argv, count);
    printf("[+] Status : %d\n", status);
    return 0;
}
