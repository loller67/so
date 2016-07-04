#include "mt.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("uso: %s 127.0.0.1\n", argv[0]);
        exit(1);
    }

    char* the_ip = argv[1];

    /* Crear socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("abriendo socket");
        exit(1);
    }

    /* Crear nombre de la conexion */
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    inet_aton(the_ip, &name.sin_addr);
    name.sin_port = htons(PORT);

    /* Connect */
    if (connect(sock, (struct sockaddr*) &name, sizeof(name))) {
        perror("connecting");
        exit(1);
    }

    /* Buffer para leer mensajes */
    char buffer[MAX_MSG_LENGTH];
    for (;;) {
        fgets(buffer, MAX_MSG_LENGTH, stdin);
        write(sock, (void*) buffer, MAX_MSG_LENGTH);
        if (!strcmp(buffer, END_STRING))
            break;
        int read_size = read(sock, (void*) buffer, MAX_MSG_LENGTH);
        printf("%.*s\n", read_size, buffer);
    }

    /* Cerrar socket */
    close(sock);

    return 0;
}
