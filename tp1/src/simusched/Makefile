export LC_ALL=C
CXXFLAGS= -Wall -pedantic -g -ggdb
LDFLAGS= -lpthread

OBJS=main.o simu.o basesched.o basetask.o tasks.o sched_rr.o sched_fcfs.o sched_rr2.o sched_mistery.o sched_no_mistery.o
MAIN=simusched

.PHONY: all clean new
all: $(MAIN)

$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp %.h basesched.h
basesched.o: basesched.cpp basesched.h
basetask.o: basetask.cpp basetask.h
main.o: main.cpp simu.h basetask.h basesched.h tasks.h sched_fcfs.h \
 sched_rr.h sched_rr2.h
simu.o: simu.cpp simu.h basetask.h basesched.h
simusched.o: simusched.cpp basetask.h basesched.h tasks.h
tasks.o: tasks.cpp tasks.h basetask.h

clean:
	rm -f $(filter-out sched_mistery.o, $(OBJS)) $(MAIN)

new: clean all

ejercicio1: $(MAIN)
	./simusched lotes/lote_ej_1.tsk 1 1 1 SchedFCFS 1 | python graphsched.py > Ejercicio_1.png

ejercicio2: $(MAIN)
	./simusched lotes/lote_ej_2.tsk 1 5 1 SchedFCFS 1 | python graphsched.py > Ejercicio_2_FCFS_1_core.png
	./simusched lotes/lote_ej_2.tsk 2 5 1 SchedFCFS 1 | python graphsched.py > Ejercicio_2_FCFS_2_cores.png
	./simusched lotes/lote_ej_2.tsk 4 5 1 SchedFCFS 1 | python graphsched.py > Ejercicio_2_FCFS_4_cores.png

ejercicio3: $(MAIN)
	./simusched lotes/lote_ej_3.tsk 1 1 0 SchedFCFS | python graphsched.py > Ejercicio_3.png

ejercicio5: $(MAIN)
	./simusched lotes/lote_ej_5.tsk 1 2 0 SchedRR 2 | python graphsched.py > Ejercicio_5_1.png
	./simusched lotes/lote_ej_5.tsk 1 2 0 SchedRR 10 | python graphsched.py > Ejercicio_5_2.png
	./simusched lotes/lote_ej_5.tsk 1 2 0 SchedRR 30 | python graphsched.py > Ejercicio_5_3.png

ejercicio6: $(MAIN)
	./simusched lotes/lote_ej_5.tsk 1 2 0 SchedFCFS 0 | python graphsched.py > Ejercicio_6_1.png

ejercicio7: $(MAIN)
	./simusched lotes/lote_ej_7_1.tsk 1 1 1 SchedMistery 4 | python graphsched.py > Ejercicio_7_1_Mistery.png
	./simusched lotes/lote_ej_7_1.tsk 1 1 1 SchedNoMistery 4 | python graphsched.py > Ejercicio_7_1_NoMistery.png
	./simusched lotes/lote_ej_7_2.tsk 1 1 1 SchedMistery 1 2 3 4 5 6 | python graphsched.py > Ejercicio_7_2_Mistery.png
	./simusched lotes/lote_ej_7_2.tsk 1 1 1 SchedNoMistery 1 2 3 4 5 6 | python graphsched.py > Ejercicio_7_2_NoMistery.png
	./simusched lotes/lote_ej_7_3.tsk 1 1 1 SchedMistery 5 2 10 1 | python graphsched.py > Ejercicio_7_3_Mistery.png
	./simusched lotes/lote_ej_7_3.tsk 1 1 1 SchedNoMistery 5 2 10 1 | python graphsched.py > Ejercicio_7_3_NoMistery.png

ejercicio8: $(MAIN)
	./simusched lotes/lote_ej_8_1.tsk 2 1 2 SchedRR2 10 10 | python graphsched.py > ej_8_1_sin_migracion.png
	./simusched lotes/lote_ej_8_1.tsk 2 1 2 SchedRR 10 10 | python graphsched.py > ej_8_1_con_migracion.png
	./simusched lotes/lote_ej_8_2.tsk 2 1 4 SchedRR2 10 10 | python graphsched.py > ej_8_2_sin_migracion.png
	./simusched lotes/lote_ej_8_2.tsk 2 1 4 SchedRR 10 10 | python graphsched.py > ej_8_2_con_migracion.png
