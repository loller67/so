#ifndef RWLock_h
#define RWLock_h

#include <iostream>
#include <pthread.h>

class RWLock {
    public:
        RWLock();
        void rlock();
        void wlock();
        void runlock();
        void wunlock();
    private:
        /** Indica si la seccion critica esta vacia */
        bool room_is_empty;
        /**
         * Variable de condicion que sera signaleada cuando la
         *  seccion critica este vacia.
         */
        pthread_cond_t room_empty;
        /** Mutex que acompaña a la variable de condicion room_empty */
        pthread_mutex_t room_empty_mutex;
        /** Lleva la cuenta de los lectores en la seccion critica */
        int readers;
        /** Mutex que protege a la variable readers */
        pthread_mutex_t mutex_readers;
        /** Variable que funciona como molinete para los lectores y como mutex
        para los escritores */
        pthread_mutex_t turnstile;
};

#endif
