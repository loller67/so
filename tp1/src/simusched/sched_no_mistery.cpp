#include <vector>
#include <queue>
#include "sched_no_mistery.h"
#include "basesched.h"
#include <iostream>

using namespace std;

SchedNoMistery::SchedNoMistery(vector<int> argn)
{
    // MFQ recibe los quantums por parÃ¡metro

    int cantQueues = argn.size(); //Hay una cola con quantum 1 con maxima prioridad
	
	
    for(int i = 0; i< cantQueues; i++)
    {
		vq.push_back(std::queue<int>());
		def_quantum.push_back(argn[i]);
    }

    n               = cantQueues;           //Cant de colas. 
    cur_pri         = -1;                   //El proceso actual es el IDLE
    quantum         = -1;
    cantidadProcesosActivos = 0;
}

void SchedNoMistery::load(int pid)
{
    //Lo encolo a la cola para su primer ejecucion
    vq[0].push(pid);
    unblock_to.push_back(0); //Agrego una posicion al vector
    cantidadProcesosActivos++;
}

void SchedNoMistery::unblock(int pid)
{
    //Encolo el proceso a la queue de una prioridad mayor a la que tenia
    int pushTo = unblock_to[pid];
    vq[pushTo].push(pid);
    cantidadProcesosActivos++;
    
}

int SchedNoMistery::tick(int cpu, const enum Motivo m)
{
    int next_task = IDLE_TASK;

    switch (m)
    {
        case TICK:
        {
            quantum--;
			
            if ( current_pid(cpu) == IDLE_TASK ) //Si la tarea es idle
            {
                next_task = next();     //Puede ser idle la siguiente
                
            }
            else    //Si la tarea no es idle
            {
                if( quantum <= 0) //Se le acabo el tiempo asignado y hay mas de un solo proceso
                {
					if(cantidadProcesosActivos > 1)
					{
						int prior = cur_pri;

						encolar(prior, cpu);
						next_task = next(); //La siguiente tarea es next
	
					}
					else
					{
						int tmp = std::min(n-1, cur_pri +1); 
						cur_pri = tmp;
						quantum = def_quantum[cur_pri];
						next_task = current_pid(cpu);
					}
                }
                else
                {
                    next_task = current_pid(cpu);
                }
            }

            break;
        }
        case BLOCK: //Bloqueo la tarea y ejecuto la siguiente
        {
            cantidadProcesosActivos--;
            
            blockearProceso(cpu);

            next_task = next();

            break;
        }
        case EXIT: //el proceso actual termino
        {
            cantidadProcesosActivos--;
            next_task = next();
            break;
        }
        default: 
        {
            // este caso no existe, lo agrego para evitar warnings del gcc
            break;
        }
    }
    
    return next_task;


}

//next setea los tiempos de cpu que pueden usarse y la variable cur_pri (current_priority)
int SchedNoMistery::next(void)
{
    // Elijo el nuevo pid
    //si no hay devuelvo idle

    int task = IDLE_TASK;
    quantum = -1;
    cur_pri = -1;

	//Elijo al de mayor prioridad
    for(int i = 0; i < n; i++)
	{
		if( !vq[i].empty() )
		{
			quantum = def_quantum[i];
			task = vq[i].front();
			vq[i].pop(); //Desencolo y retorno el pid que tiene que seguir
			cur_pri = i;
			
			break;
		}
	}

    return task;
}


//Auxiliar para que quede mas lindo el codigo
void SchedNoMistery::blockearProceso(int cpu)
{
    if( cur_pri <= 1) //Si esta en la maxima prioridad o en la queue de los recien inicializados
    {
        unblock_to[current_pid(cpu)] = 0;
    }
    else //la prioridad esta entre 0 y n no inclusives
    {
		cur_pri--;
        unblock_to[current_pid(cpu)] = cur_pri;
    }
}


void SchedNoMistery::encolar(int prior, int cpu)
{
    vq[ std::min( n-1 , prior + 1 ) ].push(current_pid(cpu)); 
}


