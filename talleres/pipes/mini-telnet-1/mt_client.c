#include "mt.h"

int main(int argc, char* argv[]) {

    /* Crear socket */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("abriendo socket");
        exit(1);
    }

    /* Crear nombre */
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    /* Conectar */
    if (connect(sock, (struct sockaddr*) &name, sizeof(name))) {
        perror("connecting");
        exit(1);
    }

    /* Buffer para leer mensajes */
    char buffer[MAX_MSG_LENGTH];
    while (1) {
        fgets(buffer, MAX_MSG_LENGTH, stdin);
        write(sock, (void*) buffer, MAX_MSG_LENGTH);
        if (!strcmp(buffer, END_STRING))
            break;
    }

    /* Cerrar socket */
    close(sock);

    return 0;
}
