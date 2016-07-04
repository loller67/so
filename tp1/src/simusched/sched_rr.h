#ifndef __SCHED_RR__
#define __SCHED_RR__

#include <vector>
#include <queue>
#include "basesched.h"

class SchedRR : public SchedBase {
    public:
        SchedRR(std::vector<int> argn);
        ~SchedRR();
        virtual void load(int pid);
        virtual void unblock(int pid);
        virtual int tick(int cpu, const enum Motivo m);

    private:
        /* Cantidad de cores */
        int cores_count;
        /* Cola de tareas en estado ready */
        std::queue<int> ready_queue;
        /* Quantum de cada cpu */
        std::vector<int> cpu_quantum;
        /* Contador de los ticks gastados por las tareas en cada core */
        std::vector<int> used_quantum;
        int next(int cpu);
};

#endif
