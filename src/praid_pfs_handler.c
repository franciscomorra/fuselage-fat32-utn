/*
 * praid_pfs_handler.c
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */

#include "praid_pfs_handler.h"
#include "praid_ppdlist.h"
#include "praid_queue.h"
#include "praid_pfslist.h"
#include <stdint.h>
#include <pthread.h>
#include "tad_queue.h"
#include <assert.h>

extern queue_t ppdlist;
extern pthread_mutex_t ppdlist_mutex;
extern queue_t pfslist;
extern queue_t responselist;
extern uint32_t sync_write_count;
extern pthread_mutex_t sync_mutex;


void* PFSHANDLER_sendResponse(uint32_t ppd_fd,char* msg)
{
	uint32_t sector = *((uint32_t*) (msg+7));
	uint16_t msg_len = *((uint16_t*) (msg+1));
	uint32_t request_id = *((uint32_t*) (msg+3));

	pfs_response_t *response_found = PFSRESPONSE_searchAndTake(&responselist,request_id,sector);

	//assert(response_found->request_id == 0);
	if (response_found != NULL)
	{
		if (response_found->write_count > 0)
		{
				response_found->write_count--;
				QUEUE_appendNode(&responselist,response_found);
		}

		if (response_found->write_count == 0)
		{
			if (response_found->sync_write_response == false && response_found->request_id == 0)
			{
				pthread_mutex_lock(&ppdlist_mutex);
				ppd_node_t *ppd = PPDLIST_getByFd(ppdlist,response_found->pfs_fd);
				pthread_mutex_unlock(&ppdlist_mutex);

				pfs_request_t *new_pfsrequest = malloc(sizeof(pfs_request_t));

				msg[0] = WRITE_SECTORS;
				new_pfsrequest->request_id = 0;
				new_pfsrequest->msg = malloc(msg_len+3);
				memcpy(new_pfsrequest->msg,msg,msg_len+3);
				new_pfsrequest->pfs_fd = response_found->pfs_fd;

				pthread_mutex_lock(&ppd->request_list_mutex);
				QUEUE_appendNode(&ppd->request_list,new_pfsrequest);
				sem_post(&ppd->request_list_sem);
				pthread_mutex_unlock(&ppd->request_list_mutex);
			}
			else if (response_found->sync_write_response == true && response_found->request_id == 0)
			{
				sync_write_count--;
				if (sync_write_count == 0)
				{
					pthread_mutex_unlock(&sync_mutex);
					//HABILITAR DISCO, TERMINO DE SINCRONIZAR
				}

			}
			else
			{
				pfs_node_t* pfs = PFSLIST_getByFd(pfslist,response_found->pfs_fd);


				 fd_set write_set;
				 FD_ZERO(&write_set);
				 FD_SET(response_found->pfs_fd,&write_set);
				 select(response_found->pfs_fd+1,NULL,&write_set,NULL,NULL);

				 pthread_mutex_lock(&pfs->sock_mutex);
				 int32_t sent = 0;

		 			if(FD_ISSET(response_found->pfs_fd,&write_set))
 						sent = COMM_send(msg,response_found->pfs_fd);

				pthread_mutex_unlock(&pfs->sock_mutex);
			}
		}
		free(response_found);
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


void PFSREQUEST_removeAll()
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		queueNode_t* cur_request_node = cur_ppd->request_list.begin;

		while (cur_request_node != NULL)
		{
			free(((pfs_request_t*) cur_request_node->data)->msg);
			free(cur_request_node->data);
			free(cur_request_node);
			cur_request_node = cur_request_node->next;
		}

		cur_ppd_node = cur_ppd_node->next;
	}
}

pfs_response_t* PFSRESPONSE_searchAndTake(queue_t* response_list,uint32_t request_id,uint32_t sector)
{
	queueNode_t *cur_node = response_list->begin;
	queueNode_t *prev_node = cur_node;
	while (cur_node != NULL)
	{
		pfs_response_t *cur_response = (pfs_response_t*) cur_node->data;

		if (cur_response->sector == sector && cur_response->request_id == request_id)
		{
			if (prev_node == cur_node)
			{
				response_list->begin = cur_node->next;
				if (response_list->begin == NULL) response_list->end = NULL;
			}
			else
			{
				prev_node->next = cur_node->next;
				if (prev_node->next == NULL) response_list->end = prev_node;
			}
			free(cur_node);
			return cur_response;
		}
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	return NULL;
}

bool PFSRESPONSE_search(queue_t* response_list,uint32_t request_id,uint32_t sector)
{
	queueNode_t *cur_node = response_list->begin;

	while (cur_node != NULL)
	{
		pfs_response_t *cur_response = (pfs_response_t*) cur_node->data;

		if (cur_response->sector == sector && cur_response->request_id == request_id)
		{
			return true;
		}
		cur_node = cur_node->next;
	}
	return false;
}
