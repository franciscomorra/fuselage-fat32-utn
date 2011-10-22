/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_comm.h"
#include "tad_queue.h"
#include "praid_queue.h"


extern uint32_t raid_status; //0 INACTIVE - 1 ACTIVE
extern uint32_t ppd_thread_amount; // CONTADOR DE THREADS DE PPD

extern queue_t colaREAD;
extern queue_t colaWrite;

extern pthread_mutex_t mutex_READ;
extern pthread_mutex_t mutex_WRITE;

void *ppd_handler_thread (void *data)
{
	uint32_t ppd_status = 0; //0-Nuevo 1-Sincronizado 2-Esperando Write 3-Esperando Read

	// TODO Sincronizarlo (Crear thread de sincronizacion), que cuando sincronize que cambie el estado

	while(ppd_status != 0){
		ppd_status = handle_WRITE_request();
	}

return NULL;
}

uint32_t handle_WRITE_request()
{
/*
	pthread_mutex_lock(&mutex_WRITE);

	queueNode_t* writeRequestNode = colaWrite->begin;//Tomar el nodo y leerlo
	if (writeRequestNode != NULL){
		write_node_content writeRequest;
	//	write_node_content writeRequest = writeRequestNode->data;

		if (writeRequest.threads_left == ppd_thread_amount){ //Es el primero en leerlo
			writeRequest.threads_left--;
		}else if(writeRequest.threads_left == 1){//Es el ultimo en leerlo
			QUEUE_destroyNode(writeRequestNode,writeRequest.msg.type);
		}
		//Send to PPD
		//ppd_status = 2-Esperando Write

		//Disminuir la cantidad de threads que lo leyeron. Ver que se hace cuando es 1


		//Volverlo a poner en la cola al principio (no me sirve el take)
	}
	pthread_mutex_unlock(&mutex_WRITE);
	//Esperar respuesta?
	//Enviar a PFS si habia sido el primero en leerlo
	//ppd_status = 1;
*/
	handle_READ_request();
return 0;
}


uint32_t handle_READ_request()//
{
	/*
	pthread_mutex_lock(&mutex_READ);
	queueNode_t* readRequestNode = QUEUE_takeNode(colaREAD);
	read_node_content readRequest = readRequestNode->data;
	QUEUE_destroyNode(readRequestNode,readRequest.msg.type);

	pthread_mutex_unlock(&mutex_READ);
	if (readRequest != NULL){
		//Send to PPD
		//ppd_status = 3-Esperando Read
	}

	//Esperar respuesta?
	//Enviar a PFS
	//ppd_status = 1;
	*/
	return 0;
}
