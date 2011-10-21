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
#include "praid_queue.h"


extern uint32_t raid_status; //0 INACTIVE - 1 ACTIVE
extern uint32_t ppd_thread_amount; // CONTADOR DE THREADS DE PPD

extern read_node read_first;
extern read_node read_last;
extern write_node write_first;
extern write_node write_last;

extern pthread_mutex_t mutex_READ;
extern pthread_mutex_t mutex_WRITE;

extern uint32_t praid_ppd_status;//0-Nuevo 1-Listo 2-Esperando Write 3-Esperando Read


void *ppd_handler_thread (void *data)
{
uint32_t praid_ppd_status = 0; //0-Nuevo 1-Sincronizado 2-Esperando Write 3-Esperando Read
while (1){
	print_Console("Arrancando Nuevo Thread PPD");
	printf("The ID of this thread is: %u\n", (unsigned int)pthread_self());

}
// TODO Sincronizarlo (Crear thread de sincronizacion)

	while(praid_ppd_status != 0){
		handle_WRITE_request();
	}

return NULL;
}

uint32_t handle_WRITE_request()
{
	//Tomar el nodo y leerlo
	//Disminuir la cantidad de threads que lo leyeron. Ver que se hace cuando es 1
	//praid_ppd_status = 2-Esperando Write
	//Enviar a PPD
	//Esperar respuesta?
	//Enviar a PFS si habia sido el primero en leerlo
	//praid_ppd_status = 1;

	handle_READ_request();
return 0;
}


uint32_t handle_READ_request()//
{
	//Tomar el nodo, leerlo
	//Eliminar nodo
	//praid_ppd_status = 3-Esperando Read
	//Enviar a PPD
	//Esperar respuesta?
	//Enviar a PFS
	//praid_ppd_status = 1;

	return 0;
}

//TODO send to PFS mandale el NIPC, ver bien por el tema de sockets, habra que agregar un semaforo ahi?
//TODO send to PPD mandale el NIPC, espera a que te responda ver bien tema de sockets
//cambia tu propio estado READY, SINCRONIZE, WAIT_WRITE, WAIT_READ
