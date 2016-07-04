#include "mt.h"

int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in name, client;
    char buf[MAX_MSG_LENGTH];

    /* Crear socket: dominio INET, protocolo TCP (SOCK_STREAM) */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("abriendo socket");
        exit(1);
    }

    /* Crear nombre, INADDR_ANY indica que cualquiera puede enviarme */
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    /* Bind */
    if (bind(sock, (void*) &name, sizeof(name))) {
        perror("binding socket");
        exit(1);
    }

    /* Listen */
    if (listen(sock, 5)) {
        perror("listening");
        exit(1);
    }

    /* Accept */
    int client_size = sizeof(client);
    int client_socket = accept(sock,
        (struct sockaddr*) &client,
        (socklen_t*) &client_size);

    if (client_socket < 0) {
        perror("accepting");
        exit(1);
    }

    /* Redirigir stdout y stderr al socket del cliente */
    dup2(client_socket, 1);
    dup2(client_socket, 2);

    /* Recibir mensajes hasta que alguno sea 'chau' */
    for (;;) {
        memset(buf, 0, MAX_MSG_LENGTH);
        read(client_socket, buf, MAX_MSG_LENGTH);
        if (strncmp(buf, END_STRING, MAX_MSG_LENGTH) == 0)
            break;
        printf("Comando: %s", buf);
        system(buf);
    }

    /* Cerrar socket de recepción. */
    close(client_socket);
    close(sock);

    return 0;
}
