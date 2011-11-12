/*
 * praid_pfs_handler.c
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */

#include "praid_pfs_handler.h"
#include <stdint.h>
#include <pthread.h>
#include "tad_queue.h"

extern queue_t responselist;
extern pthread_mutex_t responselist_mutex;

void* PFSHANDLER_sendResponse(uint32_t ppd_fd,char* msg)
{
	uint32_t sector = *((uint32_t*) (msg+3));
	uint16_t msg_len = *((uint16_t*) (msg+1));
	queueNode_t *cur_response =	responselist.begin;
	queueNode_t *next_response = cur_response->next;

	pfs_response_t *first_response = (pfs_response_t*) cur_response->data;
	if (first_response->sector == sector && first_response->ppd_fd == ppd_fd)
	{
		if (first_response->sync_response == true) msg[0] = WRITE_SECTORS;
		if (send(first_response->pfs_fd,msg,msg_len+3) != msg_len+3)
		{
				uint32_t santi = 2;	//ERROR AL ENVIAR
		}
		responselist.begin = next_response;
		if (responselist.begin == NULL) responselist.end = NULL;
		free(cur_response->data);
		free(cur_response);
	}
	else
	{
		while (next_response != NULL)
		{
			pfs_response_t *response = (pfs_response_t*) next_response->data;
			if (response->sector == sector && response->ppd_fd == ppd_fd)
			{
				if (response->sync_response == true) msg[0] = WRITE_SECTORS;
				//TODO VER QUE HACER CON LAS RESPUESTAS DEL DISCO LUEGO DE LA SYNC
				if (send(response->pfs_fd,msg,msg_len+3) != msg_len+3)
				{
					uint32_t santi = 2;
				}

				cur_response->next = next_response->next;
				if (cur_response->next == NULL) responselist.end = cur_response;
				free(next_response->data);
				free(next_response);
				break;
			}
			cur_response = next_response;
			next_response = next_response->next;
		}
	}
}

void PFSRESPONSE_addNew(uint32_t sector, uint32_t ppd_fd,uint32_t pfs_fd)
{
	pfs_response_t *new_response = malloc(sizeof(pfs_response_t));
	new_response->pfs_fd = pfs_fd;
	new_response->ppd_fd = ppd_fd;
	new_response->sector = sector;
	QUEUE_appendNode(&responselist,new_response);
}
