#include "Backend_multi.h"
#include "RWLock.h"

using namespace std;


// Variables globales de la conexión
int socket_servidor = -1;

// Variables globales del juego
vector<vector<char> > tablero_equipo1; // tiene las fichas del equipo1
vector<vector<char> > tablero_equipo2; // tiene las fichas del equipo2
vector<vector<bool> > bool_tablero_equipo1; // tiene las fichas temporales del equipo1
vector<vector<bool> > bool_tablero_equipo2; // tiene las fichas temporales del equipo2
vector< vector<RWLock> > mutex_tablero1;  // protege las casillas del tablero 1
vector< vector<RWLock> > mutex_tablero2; // protege las casillas del tablero 2

// Nombres de los equipos
char* equipo1 = NULL;
char* equipo2 = NULL;
pthread_mutex_t teamsMutex = PTHREAD_MUTEX_INITIALIZER; // protege a los nombres de los equipos

// Cantidad de jugadores por equipo
pthread_mutex_t mutex_jugadores_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_jugadores_2 = PTHREAD_MUTEX_INITIALIZER;
int jugadores_equipo1 = 0;
int jugadores_equipo2 = 0;

// Cantidad de jugadores listos en cada equipo
pthread_mutex_t mutex_listos_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listos_2 = PTHREAD_MUTEX_INITIALIZER;
int listos_equipo1 = 0;
int listos_equipo2 = 0;

// Variables que indican si todos los jugadores de los equipos estan listos para la batalla
pthread_mutex_t mutex_listo_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listo_2 = PTHREAD_MUTEX_INITIALIZER;
bool listo_1 = false;
bool listo_2 = false;

// Variable que indica si el servidor sigue aceptando jugadores o no
pthread_mutex_t mutex_aceptando = PTHREAD_MUTEX_INITIALIZER;
bool aceptando = true;

// Ancho y alto del tablero
unsigned int ancho = -1;
unsigned int alto = -1;


bool cargar_int(const char* numero, unsigned int& n) {
    char *eptr;
    n = static_cast<unsigned int>(strtol(numero, &eptr, 10));
    if(*eptr != '\0') {
        cerr << "error: " << numero << " no es un número: " << endl;
        return false;
    }
    return true;
}

int main(int argc, const char* argv[]) {

    // manejo la señal SIGINT para poder cerrar el socket cuando cierra el programa
    signal(SIGINT, cerrar_servidor);

    // parsear argumentos
    if (argc < 3) {
        cerr << "Faltan argumentos, la forma de uso es:" << endl <<
        argv[0] << " N M" << endl << "N = ancho del tablero , M = alto del tablero" << endl;
        return 3;
    }
    else {
        if (!cargar_int(argv[1], ancho)) {
            cerr << argv[1] << " debe ser un número" << endl;
            return 5;
        }
        if (!cargar_int(argv[2], alto)) {
            cerr << argv[2] << " debe ser un número" << endl;
            return 5;
        }
    }

    // inicializar ambos tableros, se accede como tablero[fila][columna]
    tablero_equipo1 = vector<vector<char> >(alto);
    bool_tablero_equipo1 = vector<vector<bool> >(alto);
    for (unsigned int i = 0; i < alto; ++i) {
        tablero_equipo1[i] = vector<char>(ancho, VACIO);
        bool_tablero_equipo1[i] = vector<bool>(ancho, false);
    }

    tablero_equipo2 = vector<vector<char> >(alto);
    bool_tablero_equipo2 = vector<vector<bool> >(alto);
    for (unsigned int i = 0; i < alto; ++i) {
        tablero_equipo2[i] = vector<char>(ancho, VACIO);
        bool_tablero_equipo2[i] = vector<bool>(ancho, false);
    }

    // inicializar los locks sobre los tableros
    mutex_tablero1 = vector<vector<RWLock> >(alto);
    for (unsigned int i = 0; i < alto; ++i) {
        mutex_tablero1[i] = vector<RWLock>(ancho, RWLock());
    }

    mutex_tablero2 = vector<vector<RWLock> >(alto);
    for (unsigned int i = 0; i < alto; ++i) {
        mutex_tablero2[i] = vector<RWLock>(ancho, RWLock());
    }
    
    int socketfd_cliente, socket_size;
    struct sockaddr_in local, remoto;

    // crear un socket de tipo INET con TCP (SOCK_STREAM)
    if ((socket_servidor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Error creando socket" << endl;
    }

    // permito reusar el socket para que no tire el error "Address Already in Use"
    int flag = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    // crear nombre, usamos INADDR_ANY para indicar que cualquiera puede conectarse aquí
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(PORT);
    if (bind(socket_servidor, (struct sockaddr *)&local, sizeof(local)) == -1) {
        cerr << "Error haciendo bind!" << endl;
        return 1;
    }

    // escuchar en el socket
    if (listen(socket_servidor, 1) == -1) {
        cerr << "Error escuchando socket!" << endl;
        return 1;
    }

    // aceptar conexiones entrantes.
    socket_size = sizeof(remoto);
    while (true) {
        if ((socketfd_cliente = accept(socket_servidor, (struct sockaddr*) &remoto, (socklen_t*) &socket_size)) == -1)
            cerr << "Error al aceptar conexion" << endl;
        else {
            pthread_t new_player;
            int ret_value = pthread_create(
                &new_player,
                NULL,
                (void* (*) (void*))(void*) atendedor_de_jugador,
                (void*) (long) socketfd_cliente
            );
            if (ret_value)
                cerr << "Error launching new thread!" << endl;
        }
    }

    return 0;
}

void atendedor_de_jugador(int socket_fd) {
    bool peleando = false;
    // variables locales del jugador
    char nombre_equipo[21];

     // lista de casilleros que ocupa el barco actual (aún no confirmado)
    list<Casillero> barco_actual;

    if (!aceptando_conexiones()) {
        // si deje de aceptar conexiones, mato el thread
        terminar_servidor_de_jugador(socket_fd, barco_actual, tablero_equipo1);
    }

    if (recibir_nombre_equipo(socket_fd, nombre_equipo) != 0) {
        // el cliente cortó la comunicación, o hubo un error. Cerramos todo.
        terminar_servidor_de_jugador(socket_fd, barco_actual, tablero_equipo1);
    }

    int mi_equipo = asignar_equipo(nombre_equipo);
    if (!mi_equipo)
        terminar_servidor_de_jugador(socket_fd, barco_actual, tablero_equipo1);

    if (enviar_dimensiones(socket_fd) != 0) {
        // se produjo un error al enviar. Cerramos todo.
        terminar_servidor_de_jugador(socket_fd, barco_actual, tablero_equipo1);
    }

    cout << "Esperando que juegue " << nombre_equipo << endl;

    // Veo de que equipo soy
    bool soy_equipo_1 = (mi_equipo == 1);

    // Punteros a los tableros y sus respectivos locks
    vector<vector<char> > *tablero_jugador, *tablero_rival;
    vector<vector<bool> > *bool_tablero_jugador;
    vector<vector<RWLock> > *mutex_tablero_jugador, *mutex_tablero_rival;

    // Veo que tablero usar dependiendo el equipo que soy
    if (soy_equipo_1) {
        tablero_jugador = &tablero_equipo1;
        bool_tablero_jugador = &bool_tablero_equipo1;
        mutex_tablero_jugador = &mutex_tablero1;
        tablero_rival = &tablero_equipo2;
        mutex_tablero_rival = &mutex_tablero2;
    }
    else {
        tablero_jugador = &tablero_equipo2;
        bool_tablero_jugador = &bool_tablero_equipo2;
        mutex_tablero_jugador = &mutex_tablero2;
        tablero_rival = &tablero_equipo1;
        mutex_tablero_rival = &mutex_tablero1;
    }

    while (true) {

        // espera un barco o confirmación de juego
        char mensaje[MENSAJE_MAXIMO+1];
        int comando = recibir_comando(socket_fd, mensaje);

        if (comando == MSG_PARTE_BARCO) {
            Casillero ficha;

            //Si estoy peleando, no acepto barcos ya
            if (peleando) {
                if (enviar_error(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }
                continue;
            }

            if (parsear_barco(mensaje, ficha) != 0) {
                // no es un mensaje PARTE_BARCO bien formado, hacer de cuenta que nunca llegó
                continue;
            }

            // pido un lock de escritura sobre el casillero, para garantizarme acceso exclusivo en caso de
            // necesitar escribirle el barco
            (*mutex_tablero_jugador)[ficha.fila][ficha.columna].wlock();

            // ficha contiene el nuevo barco a colocar
            if (es_ficha_valida(ficha, barco_actual, *bool_tablero_jugador, *mutex_tablero_jugador)) {
                                
                // La escribo en el tablero de los temporales
                (*bool_tablero_jugador)[ficha.fila][ficha.columna] = true;
                
                // libero el lock de escritura sobre la casilla del tablero
                (*mutex_tablero_jugador)[ficha.fila][ficha.columna].wunlock();
                
                // si la ficha es valida, pusheo la ficha al barco sin terminar
                barco_actual.push_back(ficha);

                // OK
                if (enviar_ok(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }
            }
            else {
                
                
                //Vacío a los casilleros temporales anteriores
                for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {

                    // pido el lock para escribir a la casilla del tablero
                    (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wlock();

                    // escribo el contenido del casillero al tablero
                    (*bool_tablero_jugador)[casillero->fila][casillero->columna] = false;

                    // libero el lock de escritura sobre la casilla del tablero
                    (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wunlock();
                }
                
                //unlockeo el casillero
                (*mutex_tablero_jugador)[ficha.fila][ficha.columna].wunlock();
                
                // si la ficha no es valida, limpio el barco que estaba armando
                barco_actual.clear();

                // ERROR
                if (enviar_error(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }
            }
        }
        else if (comando == MSG_LISTO) {
            // cuando algun jugador termina de poner sus barcos,
            // si los dos equipos tienen jugadores se deja de aceptar conexiones
            if (cantidad_jugadores(1) > 0 && cantidad_jugadores(2) > 0)
                dejar_de_aceptar_conexiones();

            //Si ya había terminado de poner los barcos, enviar error
            if (peleando) {

                if (enviar_error(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }

            }
            else {
                // aviso que estoy listo, para incrementar el contador de mi equipo
                jugador_listo(mi_equipo);
                
                // Estamos listos para la pelea
                peleando = true;
                    
                if (enviar_ok(socket_fd) != 0){
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }

            }
        }
        else if (comando == MSG_BARCO_TERMINADO) {

            //Si estoy peleando, no acepto barcos ya
            if (peleando) {
                if (enviar_error(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }
                continue;
            }
    
            
            // las partes acumuladas conforman un barco completo, escribirlas en el tablero del jugador y borrar las partes temporales
            for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {

                // pido el lock para escribir a la casilla del tablero
                (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wlock();

                // escribo el contenido del casillero al tablero
                (*tablero_jugador)[casillero->fila][casillero->columna] = casillero->contenido;

                // libero el lock de escritura sobre la casilla del tablero
                (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wunlock();
            }
            
            
            // limpio la lista de casilleros del barco que acabo de escribir
            barco_actual.clear();

            if (enviar_ok(socket_fd) != 0) {
                // se produjo un error al enviar. Cerramos todo.
                desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
            }
        }
        else if (comando == MSG_BOMBA) {

            Casillero ficha;
            if (parsear_bomba(mensaje, ficha) != 0) {
                // no es un mensaje BOMBA bien formado, hacer de cuenta que nunca llegó
                continue;
            }

            // ficha contiene la bomba a tirar
            // verificar si se está peleando y si es una posición válida del tablero
            if (equipo_listo(1) && equipo_listo(2) && ficha.fila <= alto - 1 && ficha.columna <= ancho - 1) {

                // pido un lock de escritura sobre el casillero correspondiente
                (*mutex_tablero_rival)[ficha.fila][ficha.columna].wlock();

                //Si había un BARCO, pongo una BOMBA
                char contenido = (*tablero_rival)[ficha.fila][ficha.columna];

                if(contenido == BARCO) {

                    (*tablero_rival)[ficha.fila][ficha.columna] = BOMBA;

                    // libero el lock de escritura sobre el casillero correspondiente
                    (*mutex_tablero_rival)[ficha.fila][ficha.columna].wunlock();

                    if (enviar_golpe(socket_fd) != 0) {
                        // se produjo un error al enviar. Cerramos todo.
                        desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                        terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                    }

                }
                else if(contenido == BOMBA) {
                    // libero el lock de escritura sobre el casillero correspondiente
                    (*mutex_tablero_rival)[ficha.fila][ficha.columna].wunlock();

                    // OK
                    if (enviar_estaba_golpeado(socket_fd) != 0) {
                        // se produjo un error al enviar. Cerramos todo.
                        desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                        terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                    }
                }
                else {
                    // libero el lock de escritura sobre el casillero correspondiente
                    (*mutex_tablero_rival)[ficha.fila][ficha.columna].wunlock();

                    // OK
                    if (enviar_ok(socket_fd) != 0) {
                        // se produjo un error al enviar. Cerramos todo.
                        desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                        terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                    }
                }
            }
            else {
                // ERROR
                if (enviar_error(socket_fd) != 0) {
                    // se produjo un error al enviar. Cerramos todo.
                    desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                    terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
                }
            }

        }
        else if (comando == MSG_UPDATE) {
            if (enviar_tablero(socket_fd, mi_equipo, peleando) != 0) {
                // se produjo un error al enviar. Cerramos todo.
                desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
                terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
            }
        }
        else if (comando == MSG_INVALID) {
            // no es un mensaje válido, hacer de cuenta que nunca llegó
            continue;
        }
        else {
            // se produjo un error al recibir. Cerramos todo.
            desarmarBarco(barco_actual, mutex_tablero_jugador, bool_tablero_jugador);
            terminar_servidor_de_jugador(socket_fd, barco_actual, *tablero_jugador);
        }
    }
    
}


// mensajes recibidos por el server

int recibir_nombre_equipo(int socket_fd, char* nombre) {
    char buf[MENSAJE_MAXIMO+1];

    if (recibir(socket_fd, buf) != 0) {
        return -1;
    }

    int res = sscanf(buf, "EQUIPO %20s", nombre);

    if (res == EOF || res != 1) {
        cerr << "ERROR: no se pudo leer el nombre del equipo" << endl;
        return -1;
    }

    return 0;
}

// informa el tipo de comando recibido (o si es inválido)
// deja el mensaje en mensaje por si necesita seguir parseando
int recibir_comando(int socket_fd, char* mensaje) {
    if (recibir(socket_fd, mensaje) != 0) {
        return -1;
    }

    char comando[MENSAJE_MAXIMO];
    sscanf(mensaje, "%s", comando);

    if (strcmp(comando, "PARTE_BARCO") == 0) {
        // el mensaje es PARTE_BARCO
        return MSG_PARTE_BARCO;
    }
    else if (strcmp(comando, "BARCO_TERMINADO") == 0) {
        // el mensaje es BARCO_TERMINADO
        return MSG_BARCO_TERMINADO;
    }
    else if (strcmp(comando, "LISTO") == 0) {
        // el mensaje es LISTO
        return MSG_LISTO;
    }
    else if (strcmp(comando, "BOMBA") == 0) {
        // el mensaje es BOMBA
        return MSG_BOMBA;
    }
    else if (strcmp(comando, "UPDATE") == 0) {
        // el mensaje es UPDATE
        return MSG_UPDATE;
    }
    else {
        cerr << "ERROR: mensaje no válido" << endl;
        return MSG_INVALID;
    }
}

int parsear_bomba(char* mensaje, Casillero& ficha) {

    int cant = sscanf(mensaje, "BOMBA %d %d", &ficha.fila, &ficha.columna);
    ficha.contenido = BOMBA;

    if (cant == 2) {
        //El mensaje BARCO es válido
        return 0;
    }
    else {
        cerr << "ERROR: " << mensaje << " no está bien formado. Debe ser BOMBA <fila> <columna>" << endl;
        return -1;
    }
}

int parsear_barco(char* mensaje, Casillero& ficha) {

    int cant = sscanf(mensaje, "PARTE_BARCO %d %d", &ficha.fila, &ficha.columna);
    ficha.contenido = BARCO;

    if (cant == 2) {
        //El mensaje PARTE_BARCO es válido
        return 0;
    }
    else {
        cerr << "ERROR: " << mensaje << " no está bien formado. Debe ser PARTE_BARCO <fila> <columna>" << endl;
        return -1;
    }
}



// mensajes enviados por el server

int enviar_dimensiones(int socket_fd) {
    char buf[MENSAJE_MAXIMO+1];
    sprintf(buf, "TABLERO %d %d", ancho, alto);
    return enviar(socket_fd, buf);
}

/**
 * Envia el tablero por el socket, dependiendo del estado actual del juego,
 * del jugador y del equipo en el que está.
 */
int enviar_tablero(int socket_fd, int mi_equipo, bool peleando) {
    char buf[MENSAJE_MAXIMO+1];
    int pos;
    vector<vector<char> > *tablero, *tablero_jugador, *tablero_rival;
    vector<vector<RWLock> > *mutex_tablero, *mutex_tablero_jugador, *mutex_tablero_rival;

    // dependiendo del equipo del jugador, asigno los punteros tablero_jugador
    // tablero_rival, mutex_tablero_jugador y mutex_tablero_rival
    if (mi_equipo == 1) {
        tablero_jugador = &tablero_equipo1;
        tablero_rival = &tablero_equipo2;
        mutex_tablero_jugador = &mutex_tablero1;
        mutex_tablero_rival = &mutex_tablero2;
    }
    else {
        tablero_jugador = &tablero_equipo2;
        tablero_rival = &tablero_equipo1;
        mutex_tablero_jugador = &mutex_tablero2;
        mutex_tablero_rival = &mutex_tablero1;
    }

    // Si los dos equipos no terminaron de poner sus barcos, muestro los
    // barcos de mi equipo
    if (!equipo_listo(mi_equipo)) {
        sprintf(buf, "BARCOS ");
        tablero = tablero_jugador;
        mutex_tablero = mutex_tablero_jugador;
        pos = 7;
    }
    else {
        //Si no, muestro los resultados de la batalla
        sprintf(buf, "BATALLA ");
        tablero = tablero_rival;
        mutex_tablero = mutex_tablero_rival;
        pos = 8;
    }

    for (unsigned int fila = 0; fila < alto; ++fila) {
        for (unsigned int col = 0; col < ancho; ++col) {
            // pido un lock de lectura sobre la posicion actual del tablero
            (*mutex_tablero)[fila][col].rlock();
            char contenido = (*tablero)[fila][col];
            switch(contenido) {
                case VACIO:
                   buf[pos] = '-';
                   break; //optional
                case BARCO:
                   //si estoy peleando, oculto los barcos. Sino, los muestro
                   buf[pos] = peleando ? '-' : 'B';
                   break; //optional
                case BOMBA:
                    buf[pos] = '*';
            }
            // libero el lock de lectura sobre la posicion actual del tablero
            (*mutex_tablero)[fila][col].runlock();

            pos++;
        }
    }
    buf[pos] = 0; //end of buffer
    cout << endl;

    return enviar(socket_fd, buf);
}

int enviar_ok(int socket_fd) {
    char buf[MENSAJE_MAXIMO+1];
    sprintf(buf, "OK");
    return enviar(socket_fd, buf);
}

int enviar_golpe(int socket_fd) {
    char buf[MENSAJE_MAXIMO+1];
    sprintf(buf, "GOLPE");
    return enviar(socket_fd, buf);
}

int enviar_estaba_golpeado(int socket_fd) {
    char buf[MENSAJE_MAXIMO+1];
    sprintf(buf, "ESTABA_GOLPEADO");
    return enviar(socket_fd, buf);
}

int enviar_error(int socket_fd) {
    char buf[MENSAJE_MAXIMO+1];
    sprintf(buf, "ERROR");
    return enviar(socket_fd, buf);
}


// otras funciones
void cerrar_servidor(int signal) {
    cout << "¡Adiós mundo cruel!" << endl;
    if (socket_servidor != -1)
        close(socket_servidor);
    exit(EXIT_SUCCESS);
}

void terminar_servidor_de_jugador(int socket_fd, list<Casillero>& barco_actual, vector<vector<char> >& tablero_cliente) {
    cout << "Se interrumpió la comunicación con un cliente" << endl;
    close(socket_fd);
    pthread_exit(NULL);
}


void quitar_partes_barco(list<Casillero>& barco_actual, vector<vector<char> >& tablero_cliente) {
    for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {
        tablero_cliente[casillero->fila][casillero->columna] = VACIO;
    }
    barco_actual.clear();
}


bool es_ficha_valida(const Casillero& ficha, const list<Casillero>& barco_actual, const vector<vector<bool> >& tablero, vector<vector<RWLock> >&mutex_tablero) {

    // si está fuera del tablero, no es válida
    if (ficha.fila > alto - 1 || ficha.columna > ancho - 1) 
    {
        return false;
    }

    // si la ficha ya estaba en la lista 'barco_actual', se toma como inválida
    // (copiando el comportamiento del backend_mono, que toma la misma decision)
    list<Casillero>::const_iterator it;
    for (it = barco_actual.begin(); it != barco_actual.end(); it++) {
        if (it->columna == ficha.columna && it->fila == ficha.fila)
            return false;
    }

    // si el casillero está ocupado, tampoco es válida
    if (tablero[ficha.fila][ficha.columna] == true) {
        return false;
    }

    if (barco_actual.size() > 0) {
        // no es la primera parte del barco, ya hay fichas colocadas para este barco
        Casillero mas_distante = casillero_mas_distante_de(ficha, barco_actual);
        int distancia_vertical = ficha.fila - mas_distante.fila;
        int distancia_horizontal = ficha.columna - mas_distante.columna;

        if (distancia_vertical == 0) {
            // el barco es horizontal
            for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {
                if (ficha.fila - casillero->fila != 0) {
                    // no están alineadas horizontalmente
                    return false;
                }
            }

            int paso = distancia_horizontal / abs(distancia_horizontal);
            for (unsigned int columna = mas_distante.columna; columna != ficha.columna; columna += paso) {
                // el casillero DEBE estar ocupado en el tablero
                if (!(puso_barco_en(ficha.fila, columna, barco_actual)) && tablero[ficha.fila][columna] == false) {
                    return false;
                }
            }

        } else if (distancia_horizontal == 0) {
            // el barco es vertical
            for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {
                if (ficha.columna - casillero->columna != 0) {
                    // no están alineadas verticalmente
                    return false;
                }
            }

            int paso = distancia_vertical / abs(distancia_vertical);
            for (unsigned int fila = mas_distante.fila; fila != ficha.fila; fila += paso) {
                // el casillero DEBE estar ocupado en el tablero
                if (!(puso_barco_en(fila, ficha.columna, barco_actual)) && tablero[fila][ficha.columna] == false) {
                    return false;
                }
            }
        }
        else {
            // no están alineadas ni horizontal ni verticalmente
            return false;
        }
    }

    return true;
}


Casillero casillero_mas_distante_de(const Casillero& ficha, const list<Casillero>& barco_actual) {
    const Casillero* mas_distante;
    int max_distancia = -1;
    for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {
        int distancia = max<unsigned int>(abs((int)(casillero->fila - ficha.fila)), abs((int)(casillero->columna - ficha.columna)));
        if (distancia > max_distancia) {
            max_distancia = distancia;
            mas_distante = &*casillero;
        }
    }

    return *mas_distante;
}


bool puso_barco_en(unsigned int fila, unsigned int columna, const list<Casillero>& barco_actual) {
    for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++) {
        if (casillero->fila == fila && casillero->columna == columna)
            return true;
    }
    // si no encontró
    return false;
}


/**
 * En base al estado actual de las variables que indican los nombres de
 * los equipos, devuelve el numero de equipo del nombre indicado por parametro
 * (1 o 2), ó 0 en caso de error (si ya existen los dos equipos).
 */
int asignar_equipo(char* nuevo_equipo) {
    int numero_equipo;
    pthread_mutex_lock(&teamsMutex);
    // si el primer nombre nunca fue seteado, devuelvo 1
    if (!equipo1) {
        equipo1 = strdup(nuevo_equipo);
        numero_equipo = 1;
    }
    // si es igual al nombre del equipo 1, devuelvo 1
    else if (!strcmp(nuevo_equipo, equipo1)) {
        numero_equipo = 1;
    }
    // si el segundo nombre nunca fue seteado, devuelvo 2
    else if (!equipo2) {
        equipo2 = strdup(nuevo_equipo);
        numero_equipo = 2;
    }
    else if (!strcmp(nuevo_equipo, equipo2)) {
        numero_equipo = 2;
    }
    else {
        // si fallaron todos los if anteriores, devuelvo 0 porque ambos equipos
        // estaban asignados y trate de registrar uno nuevo
        numero_equipo = 0;
    }
    // incremento el contador de jugadores del equipo correspondiente
    pthread_mutex_unlock(&teamsMutex);
    if (numero_equipo == 1) {
        pthread_mutex_lock(&mutex_jugadores_1);
        jugadores_equipo1++;
        pthread_mutex_unlock(&mutex_jugadores_1);
    }
    else if (numero_equipo == 2) {
        pthread_mutex_lock(&mutex_jugadores_2);
        jugadores_equipo2++;
        pthread_mutex_unlock(&mutex_jugadores_2);
    }
    return numero_equipo;
}


/**
 * Indica si sigo aceptando jugadores en el juego o no.
 */
bool aceptando_conexiones() {
    pthread_mutex_lock(&mutex_aceptando);
    bool res = aceptando;
    pthread_mutex_unlock(&mutex_aceptando);
    return res;
}


/**
 * Deja de aceptar jugadores nuevos.
 */
void dejar_de_aceptar_conexiones() {
    pthread_mutex_lock(&mutex_aceptando);
    aceptando = false;
    pthread_mutex_unlock(&mutex_aceptando);
}

/**
 * Avisa que un jugador del equipo pasado por parametro termino de poner barcos.
 * Además, si el contador de listos llega al total de jugadores del equipo,
 * setea a true el flag que indica que todo el equipo está listo.
 */
void jugador_listo(int equipo) {
    int listos_equipo, jugadores_equipo;
    if (equipo == 1) {
        // incremento el contador del equipo 1
        pthread_mutex_lock(&mutex_listos_1);
        listos_equipo1++;
        listos_equipo = listos_equipo1;
        pthread_mutex_unlock(&mutex_listos_1);

        // si todos los del equipo 1 terminaron, seteo a true
        // la variable listos_1
        jugadores_equipo = cantidad_jugadores(equipo);
        if (jugadores_equipo == listos_equipo) {
            pthread_mutex_lock(&mutex_listo_1);
            listo_1 = true;
            pthread_mutex_unlock(&mutex_listo_1);
        }
    }
    else if (equipo == 2) {
        // incremento el contador del equipo 2
        pthread_mutex_lock(&mutex_listos_2);
        listos_equipo2++;
        listos_equipo = listos_equipo2;
        pthread_mutex_unlock(&mutex_listos_2);

        // si todos los del equipo 2 terminaron, seteo a true
        // la variable listos_2
        jugadores_equipo = cantidad_jugadores(equipo);
        if (jugadores_equipo == listos_equipo) {
            pthread_mutex_lock(&mutex_listo_2);
            listo_2 = true;
            pthread_mutex_unlock(&mutex_listo_2);
        }
    }
}

/**
 * Indica si todos los jugadores del equipo terminaron de poner sus barcos.
 */
bool equipo_listo(int equipo) {
    bool res;
    if (equipo == 1) {
        pthread_mutex_lock(&mutex_listo_1);
        res = listo_1;
        pthread_mutex_unlock(&mutex_listo_1);
    }
    else if (equipo == 2) {
        pthread_mutex_lock(&mutex_listo_2);
        res = listo_2;
        pthread_mutex_unlock(&mutex_listo_2);
    }
    else {
        // este caso no existe, lo pongo para evitar warnings del gcc
        res = false;
    }
    return res;
}

/**
 * Devuelve la cantidad total de jugadores en el equipo indicado por parametro,
 * o -1 si indiqué un número de equipo distinto a 1 y 2 para indicar el error.
 */
int cantidad_jugadores(int equipo) {
    int jugadores;
    if (equipo == 1) {
        pthread_mutex_lock(&mutex_jugadores_1);
        jugadores = jugadores_equipo1;
        pthread_mutex_unlock(&mutex_jugadores_1);
    }
    else if (equipo == 2) {
        pthread_mutex_lock(&mutex_jugadores_2);
        jugadores = jugadores_equipo2;
        pthread_mutex_unlock(&mutex_jugadores_2);
    }
    else {
        // valor de retorno fruta, para indicar error en el equipo indicado
        jugadores = -1;
    }
    return jugadores;
}


void desarmarBarco(list<Casillero>& barco_actual, vector< vector<RWLock> > *mutex_tablero_jugador, vector< vector<bool> > *bool_tablero_jugador)
{   
    //Vacío a los casilleros temporales anteriores
    for (list<Casillero>::const_iterator casillero = barco_actual.begin(); casillero != barco_actual.end(); casillero++)
    {
        // pido el lock para escribir a la casilla del tablero
        (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wlock();

        // escribo el contenido del casillero al tablero
        (*bool_tablero_jugador)[casillero->fila][casillero->columna] = false;

        // libero el lock de escritura sobre la casilla del tablero
        (*mutex_tablero_jugador)[casillero->fila][casillero->columna].wunlock();
    }
    barco_actual.clear();

}
