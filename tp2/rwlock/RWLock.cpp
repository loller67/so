#include "RWLock.h"

/** Constructor */
RWLock::RWLock() {
    // inicializa en true la variable que indica el estado de la seccion critica
    room_is_empty = true;
    // inicializa el mutex que protege al contador de lectores
    pthread_mutex_init(&mutex_readers, NULL);
    // inicializa el contador de lectores presentes en la seccion critica en 0
    readers = 0;
    // inicializa el molinete
    pthread_mutex_init(&turnstile, NULL);
    // inicializa la variable de condicion con los atributos por defecto
    pthread_cond_init(&room_empty, NULL);
    // inicializa el mutex que acompaña a room_empty
    pthread_mutex_init(&room_empty_mutex, NULL);
}

/** Pide un lock de lectura sobre el recurso. */
void RWLock::rlock() {
    // paso turnstile y lo libero inmediatamente despues
    pthread_mutex_lock(&turnstile);
    pthread_mutex_unlock(&turnstile);
    // pido el mutex para verificar cuantos lectores hay en la seccion critica
    pthread_mutex_lock(&mutex_readers);
    readers++;
    // si soy el primer lector, espero hasta que se vaya el escritor,
    // si es que hay uno
    if (readers == 1) {
        pthread_mutex_lock(&room_empty_mutex);
        while (!room_is_empty)
            pthread_cond_wait(&room_empty, &room_empty_mutex);
        room_is_empty = false;
        pthread_mutex_unlock(&room_empty_mutex);
    }
    pthread_mutex_unlock(&mutex_readers);
}

/** Pide un lock de escritura sobre el recurso. */
void RWLock::wlock() {
    // trabo turnstile, para impedirle a futuros lectores
    // matarme de inanicion mientras espero mi turno
    pthread_mutex_lock(&turnstile);
    // espero a que la seccion critica este vacia
    pthread_mutex_lock(&room_empty_mutex);
    while (!room_is_empty)
        pthread_cond_wait(&room_empty, &room_empty_mutex);
    room_is_empty = false;
    pthread_mutex_unlock(&room_empty_mutex);
}

/** Libera el lock de lectura sobre el recurso. */
void RWLock::runlock() {
    pthread_mutex_lock(&mutex_readers);
    readers--;
    if (readers == 0) {
        // si soy el ultimo lector saliendo de la seccion critica,
        // aviso que esta quedando vacia
        pthread_mutex_lock(&room_empty_mutex);
        room_is_empty = true;
        pthread_cond_signal(&room_empty);
        pthread_mutex_unlock(&room_empty_mutex);
    }
    pthread_mutex_unlock(&mutex_readers);
}

/** Libera el lock de escritura sobre el recurso. */
void RWLock::wunlock() {
    // libero turnstile, para volver a dejar pasar a otros lectores/escritores
    pthread_mutex_unlock(&turnstile);
    // aviso que la seccion critica quedo libre
    pthread_mutex_lock(&room_empty_mutex);
    room_is_empty = true;
    pthread_cond_signal(&room_empty);
    pthread_mutex_unlock(&room_empty_mutex);
}
