/*
 * praid_syncronize.c
 *
 *  Created on: 28/11/2011
 *      Author: utn_so
 */

#include "praid_synchronize.h"
#include "praid_ppdlist.h"
#include <stdint.h>

extern uint32_t total_sectors;
extern uint32_t sync_write_count;

void *synchronize_thread(void *ppd_info)
{
	ppd_node_t *ppd_node = (ppd_node_t*)ppd_info;
	sync_write_count = total_sectors;

	uint32_t sector = 0;
	for (;sector < total_sectors;sector++)
	{
		char *sync_msg = malloc(11);
		*sync_msg = 0x01;
		*((uint16_t*) (sync_msg+1)) = 8;
		*((uint32_t*) (sync_msg+3)) = 0;
		*((uint32_t*) (sync_msg+7)) = sector;
		PFSREQUEST_addNew(ppd_node->ppd_fd,sync_msg);
		free(sync_msg);
	}

	return 0;
}
