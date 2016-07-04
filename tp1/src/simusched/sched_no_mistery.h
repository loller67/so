#ifndef __SCHED_NOMIS__
#define __SCHED_NOMIS__

#include <vector>
#include <queue>
#include "basesched.h"

class SchedNoMistery : public SchedBase {
  public:
    SchedNoMistery(std::vector<int> argn);
    virtual void load(int pid);
    virtual void unblock(int pid);
    virtual int tick(int cpu, const enum Motivo m);

  private:
    std::vector<std::queue<int> > vq;   //Colas de los procesos
    std::vector<int> def_quantum;       //Quantun asignado a cala cola

    std::vector<int> unblock_to;        //Cola a la que van los procesos despues de desbloquearse

    int quantum;                        //Quantum actual que queda
    int n;                              //Cantidad de colas para facilitar el codigo
    int cur_pri;                        //Current prirorityqueue
    int next(void);

    //Auxiliares para que quede codigo lindo
    void blockearProceso(int cpu);
    void encolar(int prior, int cpu);
    int cantidadProcesosActivos;
    
};

#endif
