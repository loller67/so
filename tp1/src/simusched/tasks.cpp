#include "tasks.h"
#include <stdlib.h>

using namespace std;

void TaskCPU(int pid, vector<int> params) { // params: n
    uso_CPU(pid, params[0]); // Uso el CPU n milisegundos.
}

void TaskIO(int pid, vector<int> params) { // params: ms_pid, ms_io,
    uso_CPU(pid, params[0]); // Uso el CPU ms_pid milisegundos.
    uso_IO(pid, params[1]); // Uso IO ms_io milisegundos.
}

void TaskAlterno(int pid, vector<int> params) { // params: ms_pid, ms_io, ms_pid, ...
    for(int i = 0; i < (int)params.size(); i++) {
        if (i % 2 == 0) uso_CPU(pid, params[i]);
        else uso_IO(pid, params[i]);
    }
}

void TaskConsola(int pid, vector<int> params) {
    int n = params[0];
    int b_min = params[1];
    int b_max = params[2];
    int range = b_max - b_min;
    for (int i = 0; i < n; ++i) {
        int block_period = (rand() % (range + 1) ) + b_min; //+1 porque esta incluido el borde max
        uso_IO(pid, block_period);
    }
}

void TaskBatch(int pid, vector<int> params) {
    int total_cpu = params[0];
    int cant_bloqueos = params[1];

    // blocked indica si en el instante i la tarea ejecutara un bloqueo o no
    bool* blocked = new bool[total_cpu];
    for (int i = 0; i < total_cpu; ++i)
        blocked[i] = false;

    // genero una semilla para los numeros random
    srand((unsigned) time(0));

    // se eligen cant_bloqueos instantes random para bloquearse
    for (int i = 0; i < cant_bloqueos; ++i) {
        int random = rand() % (total_cpu);
        if (blocked[random])
            i--;
        else
            blocked[random] = true;
    }

    // si el instante i esta marcado, bloquearse durante dos ticks; caso
    // contrario, usar el CPU durante uno.
    for (int i = 0; i < total_cpu; ++i) {
        if (blocked[i])
            uso_IO(pid, 2);
        else
            uso_CPU(pid, 1);
    }

    delete[] blocked;
}

void TaskDataMining(int pid, vector<int> params) { 
	//Recibe tres parámetros en el vector. Duracion de cpu y rango min y max de duración
    
    uso_CPU(pid, params[0]); // Uso el CPU n milisegundos.
    
    int b_min = params[1];
    int b_max = params[2];
    int range = b_max - b_min;
    int block_period = (rand() % (range + 1) ) + b_min; //+1 porque esta incluido el borde max
    
    uso_IO(pid, block_period);
        
    return;
}

void tasks_init(void) {
    /* Todos los tipos de tareas se deben registrar acá para poder ser usadas.
     * El segundo parámetro indica la cantidad de parámetros que recibe la tarea
     * como un vector de enteros, o -1 para una cantidad de parámetros variable. */
    register_task(TaskCPU, 1);
    register_task(TaskIO, 2);
    register_task(TaskAlterno, -1);
    register_task(TaskConsola, 3);
    register_task(TaskBatch, 2);
    
    register_task(TaskDataMining, 3);
}
