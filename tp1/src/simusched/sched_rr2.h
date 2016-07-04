#ifndef __SCHED_RR2__
#define __SCHED_RR2__

#include <vector>
#include <queue>
#include <list>
#include "basesched.h"

class SchedRR2 : public SchedBase {
    public:
        SchedRR2(std::vector<int> argn);
        ~SchedRR2();
        virtual void load(int pid);
        virtual void unblock(int pid);
        virtual int tick(int cpu, const enum Motivo m);

    private:
        /* Cantidad de cores */
        int cores_count;
        /* Quantum de cada cpu */
        std::vector<int> cpu_quantum;
        /* Contador de los ticks gastados por las tareas en cada core */
        std::vector<int> used_quantum;
        /* Cola de tareas en estado ready de cada core*/
        std::vector< std::queue<int> > ready_queue;
        /* Contador de tareas activas en cada procesador */
        std::vector<int> active_tasks_count;
        /* Lista de tareas bloqueadas, de la forma <pid, cpu>, que
        permitira restaurar a una tarea al cpu en que estaba corriendo
        cuando se desbloquee, evitando la migracion entre cores */
        std::list< std::pair<int, int> > blocked_tasks;

        int next(int cpu);
};

#endif
