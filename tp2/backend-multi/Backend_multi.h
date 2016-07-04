#ifndef BACKEND_MULTI_H
#define BACKEND_MULTI_H


#include "Encabezado.h"
#include "Casillero.h"
#include "Enviar_recibir.h"
#include "RWLock.h"

using namespace std;
bool cargar_int(const char* numero, unsigned int& n);

void atendedor_de_jugador(int socket_fd);

// mensajes recibidos por el server
int recibir_nombre_equipo(int socket_fd, char* nombre);
int recibir_comando(int socket_fd, char* mensaje);
int parsear_barco(char* mensaje, Casillero& ficha);
int parsear_bomba(char* mensaje, Casillero& ficha);


// mensajes enviados por el server
int enviar_dimensiones(int socket_fd);
int enviar_tablero(int socket_fd, int mi_equipo, bool peleando);
int enviar_ok(int socket_fd);
int enviar_error(int socket_fd);
int enviar_golpe(int socket_fd);
int enviar_estaba_golpeado(int socket_fd);


// otras funciones
void cerrar_servidor(int signal);
void terminar_servidor_de_jugador(int socket_fd, list<Casillero>& barco_actual, vector<vector<char> >& tablero_cliente);
void desarmarBarco(list<Casillero>& barco_actual, vector< vector<RWLock> > *mutex_tablero_jugador, vector< vector<bool> > *bool_tablero_jugador);

void quitar_partes_barco(list<Casillero>& barco_actual, vector<vector<char> >& tablero_cliente);
bool es_ficha_valida(const Casillero& ficha, const list<Casillero>& barco_actual, const vector<vector<bool> >& tablero, vector<vector<RWLock> >& mutex_tablero);
Casillero casillero_mas_distante_de(const Casillero& ficha, const list<Casillero>& barco_actual);
bool puso_barco_en(unsigned int fila, unsigned int columna, const list<Casillero>& barco_actual);

/** Asigna el equipo al jugador recien llegado, en base al nombre con el que ingreso al servidor */
int asignar_equipo(char* nuevo_equipo);
/** Indica si el servidor esta aceptando conexiones nuevas */
bool aceptando_conexiones();
/** Deja de aceptar conexiones de jugadores nuevos */
void dejar_de_aceptar_conexiones();
/** Avisa que un jugador del equipo indicado terminó de poner sus barcos */
void jugador_listo(int equipo);
/** Indica si el equipo indicado por parametro esta listo para la batalla */
bool equipo_listo(int equipo);
/** Devuelve la cantidad de jugadores registrados en el equipo indicado */
int cantidad_jugadores(int equipo);

#endif /* BACKEND_MULTI_H */
