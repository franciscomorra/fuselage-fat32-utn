/*
 * praid_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#include "praid_console.h"
#include "praid_listener.h"
#include "praid_main.h"
#include "praid_queue.h"

#include "config_manager.h"


//NO SE BIEN LO DEL SCOPE, ESTAS DEBERIAN SER DE SCOPE GLOBAL, QUE LO TOMEN
uint32_t raid_console = 0; //0 ENABLE - 1 DISABLE
uint32_t raid_status = 0; //0 INACTIVE - 1 ACTIVE
uint32_t ppd_thread_amount = 0; // CONTADOR DE THREADS DE PPD

read_node read_first;
read_node read_last;

write_node write_first;
write_node write_last;


void *praid_main (void *data)
{
//TODO Leer Archivo de configuracion (COPY PASTE CARULLO) Y VER SI CONSOLE ENABLE O DISABLE

/*
		pthread_t listener_thread;
		pthread_create(&listener_thread, NULL, praid_listener, NULL);
*/
return NULL;
}

