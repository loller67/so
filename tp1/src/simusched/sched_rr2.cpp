#include <vector>
#include <queue>
#include "sched_rr2.h"
#include "basesched.h"
#include <iostream>

using namespace std;

SchedRR2::SchedRR2(vector<int> argn) {
    // Round robin recibe la cantidad de cores y sus cpu_quantum por parámetro
    cores_count = argn[0];
    cpu_quantum.resize(cores_count);
    used_quantum.resize(cores_count);
    active_tasks_count.resize(cores_count);
    ready_queue.resize(cores_count);
    for (int i = 0; i < cores_count; ++i) {
        cpu_quantum[i] = argn[i+1];
        used_quantum[i] = 0;
        active_tasks_count[i] = 0;
    }
}

SchedRR2::~SchedRR2() {

}


void SchedRR2::load(int pid) {
    int i;
    int min = 0;
    for(i = 0; i < cores_count; ++i) {
        if (active_tasks_count[i] < active_tasks_count[min])
            min = i;
    }
    ready_queue[min].push(pid);
    active_tasks_count[min]++;
}

void SchedRR2::unblock(int pid) {
    int original_core;
    list< pair<int, int> >::iterator it;
    for (it = blocked_tasks.begin(); it != blocked_tasks.end(); ++it) {
        if (it->first == pid) {
            original_core = it->second;
            blocked_tasks.remove(*it);
            break;
        }
    }
    ready_queue[original_core].push(pid);
}

int SchedRR2::tick(int cpu, const enum Motivo m) {
    int next_task = -2;
    switch (m) {
        case TICK: {
            // en tick, incrementar el contador del cpu y desalojar
            // si se agoto el quantum
            if (current_pid(cpu) == IDLE_TASK) {
                if (ready_queue[cpu].empty()) {
                    next_task = IDLE_TASK;
                }
                else {
                    used_quantum[cpu] = 0;
                    next_task = ready_queue[cpu].front();
                    ready_queue[cpu].pop();
                }
            }
            else {
                used_quantum[cpu]++;
                if (used_quantum[cpu] == cpu_quantum[cpu]) {
                    used_quantum[cpu] = 0;
                    ready_queue[cpu].push(current_pid(cpu));
                    next_task = ready_queue[cpu].front();
                    ready_queue[cpu].pop();
                }
                else {
                    next_task = current_pid(cpu);
                }
            }
            break;
        }
        case BLOCK: {
            // en block, desalojar a la tarea que se bloquea, almacenar
            // el core en que estaba corriendo y pasar a la siguiente
            int pid = current_pid(cpu);
            pair<int, int> block_info = make_pair(pid, cpu);
            blocked_tasks.push_front(block_info);
            // TODO: checkear si es necesario resetear el contador
            used_quantum[cpu] = 0;
            if (ready_queue[cpu].empty()) {
                next_task = IDLE_TASK;
            }
            else {
                next_task = ready_queue[cpu].front();
                ready_queue[cpu].pop();
            }
            break;
        }
        case EXIT: {
            // si el pid actual termino, sigue la proxima tarea
            // (o la idle si no queda ninguna)
            if (ready_queue[cpu].empty()) {
                next_task = IDLE_TASK;
                active_tasks_count[cpu] = 0;
            }
            else{
                next_task = ready_queue[cpu].front();
                ready_queue[cpu].pop();
                active_tasks_count[cpu]--;
            }
            break;
        }
        default: {
            // este caso no existe, lo agrego para evitar warnings del gcc
            break;
        }
    }
    return next_task;
}

int SchedRR2::next(int cpu) {
}
