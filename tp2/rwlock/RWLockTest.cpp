#include "RWLock.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// lock de lectores/escritores, compartido por todos los threads
RWLock the_lock;

/**
 * Clase auxiliar para pasarle parametros a los threads.
 * Contiene el id del thread (numerico, autoincremental) y un file descriptor,
 * que representara el recurso por el que compiten para leer/escribir los
 * threads.
 */
class ThreadParameters
{
public:
    /** Constructor */
    ThreadParameters() {
        thread_id = 0;
        the_file = 0;
    };
    /** Destructor */
    ~ThreadParameters() {};
    /** Id del thread */
    int thread_id;
    /** File descriptor para "leer/escribir" */
    FILE* the_file;
};

/**
 * Punto de entrada de los threads para el test_mixto.
 */
void* entry_function_mixto(void* params) {

    // obtengo los parametros del thread
    ThreadParameters* parameters = (ThreadParameters*) params;
    int my_id = parameters->thread_id;
    FILE* f = parameters->the_file;

    // arbitrariamente, decido que los que tengan id multiplo de 5 son los
    // escritores, y el resto son los lectores
    bool soy_escritor = (my_id % 5) == 0;

    if (soy_escritor) {

        // pido el lock para "escribir"
        the_lock.wlock();

        // le imprimo cosas al archivo de salida
        fprintf(f, "Soy el thread %d entrando a escribir\n", my_id);
        sleep(1);
        fprintf(f, "Soy el thread %d saliendo de escribir\n", my_id);

        // libero el lock de "escribir"
        the_lock.wunlock();
    }
    else {

        // pido el lock para "leer"
        the_lock.rlock();
        // le imprimo cosas al archivo de salida
        fprintf(f, "Soy el thread %d entrando a leer\n", my_id);
        sleep(1);
        fprintf(f, "Soy el thread %d saliendo de leer\n", my_id);
        // libero el lock de "leer"
        the_lock.runlock();
    }

    return NULL;
}

/**
 * Punto de entrada de los threads para el test_lectores.
 */
void* entry_function_lectores(void* params) {

    // obtengo los parametros del thread
    ThreadParameters* parameters = (ThreadParameters*) params;
    int my_id = parameters->thread_id;
    FILE* f = parameters->the_file;

    // pido el lock para "leer"
    the_lock.rlock();
    // le imprimo cosas al archivo de salida
    fprintf(f, "Soy el thread %d entrando a leer\n", my_id);
    sleep(1);
    fprintf(f, "Soy el thread %d saliendo de leer\n", my_id);
    // libero el lock de "leer"
    the_lock.runlock();

    return NULL;
}

/**
 * Punto de entrada de los threads para el test_escritores.
 */
void* entry_function_escritores(void* params) {

    // obtengo los parametros del thread
    ThreadParameters* parameters = (ThreadParameters*) params;
    int my_id = parameters->thread_id;
    FILE* f = parameters->the_file;

    // pido el lock para "escribir"
    the_lock.wlock();

    // le imprimo cosas al archivo de salida
    fprintf(f, "Soy el thread %d entrando a escribir\n", my_id);
    sleep(1);
    fprintf(f, "Soy el thread %d saliendo de escribir\n", my_id);

    // libero el lock de "escribir"
    the_lock.wunlock();

    return NULL;
}




/**
 * Punto de entrada del programa.
 * El RWLock se testea de la siguiente manera:
 * Se crean 'n' threads, donde algunos seran lectores y otros escritores,
 * dependiendo del test que se indique por linea de comando.
 * Se crea un archivo de salida (output.txt), que representará el recurso que
 * los threads querrán leer/escribir concurrentemente.
 * Cada thread, dependiendo de su tipo (lector/escritor) solicita el file
 * descriptor para la operación que quiera hacer con él, loggea la acción que
 * está ejecutando (comenzando/terminando con la lectura/escritura) y termina
 * la ejecución.
 * Al terminar todos los threads, el orden de las lineas en el archivo de log
 * debe indicar que:
 * - Los threads lectores pueden acceder concurrentemente al recurso.
 * - Los threads escritores tienen acceso excluyente al recurso.
 */
int main(int argc, char const *argv[])
{
    if (argc < 3) {
        printf("Uso: %s num_threads test_case\n", argv[0]);
        return 1;
    }
    int threads_count = atoi(argv[1]);

    // elegir el test a correr en funcion de la entrada
    std::string test_case(argv[2]);
    void* (*test_function) (void *) = NULL;

    if (test_case == "test_mixto") {
        test_function = &entry_function_mixto;
    }
    else if (test_case == "test_lectores") {
        test_function = &entry_function_lectores;
    }
    else if (test_case == "test_escritores") {
        test_function = &entry_function_escritores;
    }
    else {
        printf("El parametro test_case debe ser: test_mixto, test_lectores o test_escritores\n");
        return 1;
    }

    // abro el archivo en modo write, para borrar el output de alguna corrida previa
    FILE* the_file = fopen("output.txt", "w");

    // creo las estructuras para los threads
    pthread_t* threads = new pthread_t[threads_count];
    ThreadParameters* params = new ThreadParameters[threads_count];

    // lanzo los threads
    for (int i = 0; i < threads_count; ++i) {
        params[i].thread_id = i;
        params[i].the_file = the_file;
        pthread_create(&threads[i], NULL, test_function, &params[i]);
    }

    // espero a que los threads terminen
    for (int i = 0; i < threads_count; ++i) {
        pthread_join(threads[i], NULL);
    }

    // libero las estructuras de los threads
    delete[] threads;
    delete[] params;

    // cierro el archivo
    fclose(the_file);
    return 0;
}
