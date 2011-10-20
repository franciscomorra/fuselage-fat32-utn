/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "ppd_sync.h"
#include "ppd_main.h"
#include "praid_console.h"

#include "praid_queue.h"
#include "ppd_misc.h"

extern uint32_t ppd_thread_amount; // CONTADOR DE THREADS DE PPD

extern read_node read_first;
extern read_node read_last;
extern write_node write_first;
extern write_node write_last;

extern pthread_mutex_t mutex_READ;
extern pthread_mutex_t mutex_WRITE;

void *ppd_main (void *data)
{
uint32_t ppd_status = 0; //0-Nuevo 1-Sincronizado 2-Esperando Write 3-Esperando Read
while (1){
	print_Console("Arrancando Nuevo Thread PPD");
	printf("The ID of this thread is: %u\n", (unsigned int)pthread_self());

}
// TODO Sincronizarlo (Crear thread de sincronizacion)

	while(ppd_status != 0){
		handle_WRITE_request();
	}

return NULL;
}
