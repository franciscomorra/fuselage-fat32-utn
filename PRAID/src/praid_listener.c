/*
 * praid_listener.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_listener.h"
#include "praid_comm.h"
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_queue.h"
#include "tad_queue.h"

extern uint32_t raid_status; //0 INACTIVE - 1 ACTIVE
extern uint32_t ppd_thread_amount; // CONTADOR DE THREADS DE PPD

extern queue_t colaREAD;
extern queue_t colaWrite;


extern pthread_mutex_t mutex_READ;
extern pthread_mutex_t mutex_WRITE;

void *praid_listener (void *data)
{
/*
	pthread_t ppd_thread;
	pthread_create(&ppd_thread, NULL, ppd_handler_thread, NULL);
	pthread_create(&ppd_thread, NULL, ppd_handler_thread, NULL);
	pthread_create(&ppd_thread, NULL, ppd_handler_thread, NULL);
	pthread_create(&ppd_thread, NULL, ppd_handler_thread, NULL);
	//Threads PPD de prueba
	while(1){
		print_Console("Arrancando Thread Listener");
	}
*/

/*
 *
 * TODO Conexion de sockets, inicializar y escuchar
 *
 *
 * Llega conexion de PPD
 * 	pthread_t ppd_thread;
 *	pthread_create(&ppd_thread, NULL, ppd_main, NULL);
 * 	ppd_thread_amount++;
 *
 *
 * Llega conexion de PFS
 * if(raid_status = 1){
 * 		receive_pfs()
 * }else{
 * 		error()
 * }
 *
 *
 *
 *
 *
 */

return NULL;
}
