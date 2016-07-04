#ifndef __SCHED_MIS__
#define __SCHED_MIS__

#include <vector>
#include <queue>
#include "basesched.h"

class SchedMistery : public SchedBase {
  public:
    SchedMistery(std::vector<int> argn);
    virtual void load(int pid);
    virtual void unblock(int pid);
    virtual int tick(int cpu, const enum Motivo m);

  private:
    std::vector<std::queue<int> > vq;
    std::vector<int> def_quantum;
    std::vector<int> unblock_to;
    int quantum, n, cur_pri;

    int next(void);
};

#endif
