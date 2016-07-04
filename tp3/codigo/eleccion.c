#include <stdio.h>
#include "mpi.h"
#include "eleccion.h"

// tamaño del buffer para recibir una tupla <id, cl>
#define BUFFER_SIZE 2
// maximo tiempo que espero un proceso hasta decidir que murio
#define ACK_WAIT_TIME 0.1

static t_pid siguiente_pid(t_pid pid, int es_ultimo)
{
 t_pid res= 0; /* Para silenciar el warning del compilador. */

 if (es_ultimo)
    res= 1;
 else
    res= pid+1;

 return res;
}

void iniciar_eleccion(t_pid pid, int es_ultimo)
{
    // seteo el mensaje que inicia la eleccion
    int send_buffer[BUFFER_SIZE];
    send_buffer[0] = (int) pid;
    send_buffer[1] = (int) pid;

    // enviar mensaje y esperar por el ack
    int recibi_ack = 0;
    //El proximo va a ser o bien 1 o bien hay otro proceso vivo siguiente, entonces si el siguiente no es el ultimo y es un proceso muerto,
    //Busco otro proceso incrementando en 1 a proximo (proximo++), porque tengo asegurado que hay otro vivo adelante
    t_pid proximo = siguiente_pid(pid, es_ultimo);

    while (!recibi_ack) 
    {
        // envio el mensaje
        MPI_Request send_request;
        // fprintf(stderr, "Proceso %d intentando enviar mensaje al proceso %d \n", pid, proximo);
        MPI_Isend((void*) send_buffer, BUFFER_SIZE, MPI_INT, proximo, TAG_TUPLA, MPI_COMM_WORLD, &send_request);

        //Fix para cuando hay solo un proceso. Solo llegan a ser iguales en este caso porque dio la vuelta
    	if(proximo == pid)
    	{
    		// Recibo el ack y lo saco de la cola para evitar problemas
    		MPI_Request ack_request;
            MPI_Irecv((void*) NULL, 0, MPI_INT, pid, TAG_ACK, MPI_COMM_WORLD, &ack_request);
    		break;
    	}

        // inicializo el contador para esperar por el ack
        double counter = MPI_Wtime();
        double max_wait_time = MPI_Wtime() + ACK_WAIT_TIME;
        while (counter < max_wait_time)
        {
            // checkeo si me llego un TAG_TUPLA <- fix para no dejar colgados a los inicios de elección atrás mío
            int tupla_flag = 0;
            MPI_Status tupla_status;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_TUPLA, MPI_COMM_WORLD, &tupla_flag, &tupla_status);
            if (tupla_flag)
            {
                // si me llego un mensaje TAG_TUPLA mientras espero el ACK,
                // le respondo un ACK para no dejarlo colgado
                int tupla_source = tupla_status.MPI_SOURCE;
                MPI_Request ack_request;
                MPI_Isend((void*) NULL, 0, MPI_INT, tupla_source, TAG_ACK, MPI_COMM_WORLD, &ack_request);
                //fprintf(stderr, "Proceso %d recibio un TAG_TUPLA de %d mientras iniciaba eleccion \n", pid, tupla_source);


                // recibo el mensaje para sacarlo de la cola, ignorando el contenido
                MPI_Request tupla_request;
                MPI_Irecv((void*) NULL, 0, MPI_INT, tupla_source, TAG_TUPLA, MPI_COMM_WORLD, &tupla_request);
            }

            // checkeo si me llego un TAG_ACK
            int ack_flag = 0;
            MPI_Status ack_status;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_ACK, MPI_COMM_WORLD, &ack_flag, &ack_status);
            
            
            if (!ack_flag)
            {
	            // actualizo el contador
	            counter = MPI_Wtime();
            }
            else
            {
	            // fprintf(stderr, "Proceso %d recibio el ACK de %d \n", pid, proximo);

	            // si me llego un ACK, lo recibo
	            MPI_Request ack_request;
	            MPI_Irecv((void*) NULL, 0, MPI_INT, MPI_ANY_SOURCE, TAG_ACK, MPI_COMM_WORLD, &ack_request);

	            // seteo recibi_ack a true para salir del while de afuera
	            // y salgo del while de adentro con break
	            recibi_ack = 1;
	            break;
	        }
        }
        // si se vencio el contador, volver a intentar con el siguiente proceso del anillo
        proximo++;
    }
}

void eleccion_lider(t_pid pid, int es_ultimo, unsigned int timeout)
{
    static t_status status = NO_LIDER;
    double ahora = MPI_Wtime();
    double tiempo_maximo = ahora+timeout;

    while (ahora<tiempo_maximo)
    {
        // checkeo si hay un mensaje con tag 'TAG_TUPLA'
        int flag = 0;
        MPI_Status iprobe_status;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_TUPLA, MPI_COMM_WORLD, &flag, &iprobe_status);
        if (!flag)
        {	
        	/* Si no hay, sigo esperando. Actualizo valor de la hora y vuelvo al principio del while. */
        	ahora = MPI_Wtime();
            continue;
        }

        // fprintf(stderr, "Proceso %d tiene un mensaje TAG_TUPLA\n", pid);

        // al recibir un mensaje 'TAG_TUPLA' mando inmediatamente el ACK al que me lo haya mandado
        int ack_recipient = iprobe_status.MPI_SOURCE;
        MPI_Request ack_request;
        MPI_Isend((void*) NULL, 0, MPI_INT, ack_recipient, TAG_ACK, MPI_COMM_WORLD, &ack_request);
        // fprintf(stderr, "Proceso %d enviando ACK al proceso %d\n", pid, ack_recipient);

        // recibir un mensaje con tag 'TAG_TUPLA'
        int recieve_buffer[BUFFER_SIZE];
        MPI_Request recieve_request;
        MPI_Irecv((void*) recieve_buffer, BUFFER_SIZE, MPI_INT, MPI_ANY_SOURCE, TAG_TUPLA, MPI_COMM_WORLD, &recieve_request);

        // al recibir un mensaje 'TAG_TUPLA' seteo status := NO_LIDER
        status = NO_LIDER;

        // parseo el mensaje en las dos componentes de la tupla
        int id_recibido = recieve_buffer[0];
        int cl = recieve_buffer[1];

        // buffer para enviar mensajes
        int send_buffer[BUFFER_SIZE];
        int mi_id = (int) pid;

        // decidir el mensaje en funcion del current leader (cl) recibido
        if (id_recibido == mi_id) {
            // si los ids son iguales, la eleccion dio toda la vuelta y cl es el lider
            if (cl > mi_id) {
                // el lider esta mas adelante y no sabe que ganó.
                send_buffer[0] = cl;
                send_buffer[1] = cl;
            }
            else if (cl == mi_id) {
                // yo soy el lider -> no propago más mensajes.
                status = LIDER;
            }
            else {
                // la eleccion dio toda la vuelta y el líder actual tiene id
                // menor al mío -> este caso no existe.
                // fprintf(stderr, "El proceso %d cayo en un caso inexistente \n", pid);
            }
        }
        else {
            // el token debe seguir girando
            if (mi_id > cl) {
                // mi id es mas grande -> soy el proximo candidato a lider
                send_buffer[0] = id_recibido;
                send_buffer[1] = mi_id;
            }
            else {
                // mi id es mas chico -> el candidato a lider es el mismo que recibi
                send_buffer[0] = id_recibido;
                send_buffer[1] = cl;
            }
        }

        // si status != LIDER, tengo que mandarle al siguiente el mensaje con la tupla
        if (status != LIDER) {

            // flag para cortar el ciclo de espera por el ACK si ya lo recibi
            int recibi_ack = 0;
            // flag para cortar el ciclo de espera por el ACK si se consumio mi tiempo total
            int termino_mi_tiempo = 0;
            t_pid proximo = siguiente_pid(pid, es_ultimo);
            while (!recibi_ack && !termino_mi_tiempo) {

                // envio el mensaje
                MPI_Request send_request;
                // fprintf(stderr, "Proceso %d intentando enviar mensaje al proceso %d \n", pid, proximo);
                MPI_Isend((void*) send_buffer, BUFFER_SIZE, MPI_INT, proximo, TAG_TUPLA, MPI_COMM_WORLD, &send_request);

                // inicializo el contador para esperar por el ack
                double counter = MPI_Wtime();
                double max_wait_time = MPI_Wtime() + ACK_WAIT_TIME;

                while (counter < max_wait_time && counter < tiempo_maximo) {
                    
                    // checkeo si me llego un TAG_TUPLA
                    int tupla_flag = 0;
                    MPI_Status tupla_status;
                    MPI_Iprobe(MPI_ANY_SOURCE, TAG_TUPLA, MPI_COMM_WORLD, &tupla_flag, &tupla_status);

                    if (tupla_flag) {
                        // si me llego un mensaje TAG_TUPLA mientras espero el ACK,
                        // le respondo con ACK al proceso que mando la tupla
                        // para no dejarlo colgado mientras yo espero
                        int tupla_source = tupla_status.MPI_SOURCE;
                        MPI_Request ack_request;
                        MPI_Isend((void*) NULL, 0, MPI_INT, tupla_source, TAG_ACK, MPI_COMM_WORLD, &ack_request);
                        // fprintf(stderr, "Proceso %d recibio un TAG_TUPLA de %d mientras esperaba el ACK \n", pid, tupla_source);

                        // recibo el mensaje para sacarlo de la cola, ignorando el contenido
                        MPI_Request tupla_request;
                        MPI_Irecv((void*) NULL, 0, MPI_INT, tupla_source, TAG_TUPLA, MPI_COMM_WORLD, &tupla_request);
                    }

                    // checkeo si me llego un ACK
                    int ack_flag = 0;
                    MPI_Status ack_status;
                    MPI_Iprobe(MPI_ANY_SOURCE, TAG_ACK, MPI_COMM_WORLD, &ack_flag, &ack_status);
                    if (!ack_flag)
                    {	
                    	// actualizo el contador y vuelvo al comienzo del while (el interno)
                    	counter = MPI_Wtime();
                        continue;
                    }

                    // fprintf(stderr, "Proceso %d recibio el ACK de %d \n", pid, proximo);

                    // si me llego un ACK, lo recibo
                    MPI_Request ack_request;
                    MPI_Irecv((void*) NULL, 0, MPI_INT, MPI_ANY_SOURCE, TAG_ACK, MPI_COMM_WORLD, &ack_request);

                    // seteo recibi_ack a true para salir del while de afuera
                    // y salgo del while de adentro con break
                    recibi_ack = 1;
                    break;
                }

				// actualizo el contador
                counter = MPI_Wtime();
                if (counter >= tiempo_maximo)
                    // si se termino mi tiempo de ejecucion, salgo del while de afuera
                    termino_mi_tiempo = 1;
                else
                    // caso contrario, avanzo al proximo proceso del anillo
                    proximo++;
            }
        }
    }

    /* Reporto mi status al final de la ronda. */
    printf("Proceso %u %s lider.\n", pid, (status==LIDER ? "es" : "no es"));
}
