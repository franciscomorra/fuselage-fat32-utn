/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include "ppd_sync.h"
#include "ppd_main.h"

#include "praid_queue.h"
#include "ppd_misc.h"

void *ppd_main (void *data)
{
	uint32_t ppd_status = 0;
// TODO Crear thread de sincronizacion
//Esperar al fin de sincronizacion

	handle_WRITE_request();

return NULL;
}
