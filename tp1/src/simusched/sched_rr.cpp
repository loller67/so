#include <vector>
#include <queue>
#include "sched_rr.h"
#include "basesched.h"
#include <iostream>
#include <stdio.h>

using namespace std;

SchedRR::SchedRR(vector<int> argn) {
    // Round robin recibe la cantidad de cores y sus cpu_quantum por parámetro
    cores_count = argn[0];
    cpu_quantum.resize(cores_count);
    used_quantum.resize(cores_count);
    for (int i = 0; i < cores_count; ++i) {
        cpu_quantum[i] = argn[i+1];
        used_quantum[i] = 0;
    }
}

SchedRR::~SchedRR() {
}


void SchedRR::load(int pid) {
    ready_queue.push(pid);
}

void SchedRR::unblock(int pid) {
    ready_queue.push(pid);
}

int SchedRR::tick(int cpu, const enum Motivo m) {
    int next_task = -2;
    switch (m) {
        case TICK: {
            if (current_pid(cpu) == IDLE_TASK) {
                if (!ready_queue.empty()) {
                    used_quantum[cpu] = 0;
                    next_task = ready_queue.front();
                    ready_queue.pop();
                }
                else {
                    next_task = IDLE_TASK;
                }
            }
            else {
                used_quantum[cpu]++;
                if (used_quantum[cpu] == cpu_quantum[cpu]) {
                    used_quantum[cpu] = 0;
                    ready_queue.push(current_pid(cpu));
                    next_task = ready_queue.front();
                    ready_queue.pop();
                }
                else {
                    next_task = current_pid(cpu);
                }
            }
            break;
        }
        case BLOCK: {
            used_quantum[cpu] = 0;
            if (!ready_queue.empty()) {
                next_task = ready_queue.front();
                ready_queue.pop();
            }
            else {
                next_task = IDLE_TASK;
            }
            break;
        }
        case EXIT: {
            used_quantum[cpu] = 0;
            if (!ready_queue.empty()) {
                next_task = ready_queue.front();
                ready_queue.pop();
            }
            else {
                next_task = IDLE_TASK;
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
