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

extern queue_t ppd_list;
extern pthread_mutex_t ppd_list_mutex;
extern pthread_mutex_t pending_request_list_mutex;
extern queue_t pfs_list;
extern queue_t pending_request_list;
extern uint32_t pending_writes_forSyncronization;
extern pthread_mutex_t sync_mutex;
extern uint32_t ACTIVE_DISKS_AMOUNT;
extern uint32_t DISKS_AMOUNT;

void pfs_pending_request_attendTo(uint32_t ppd_fd,char* msg)
{
	uint32_t sector = *((uint32_t*) (msg+7));
	uint16_t msg_len = *((uint16_t*) (msg+1));
	uint32_t request_id = *((uint32_t*) (msg+3));

	pthread_mutex_lock(&pending_request_list_mutex);
	pfs_pending_request_t *pending_request = pfs_pending_request_searchAndTake(&pending_request_list,request_id,sector);

	//assert(response_found->request_id == 0);
	if (pending_request != NULL)
	{
		if (pending_request->write_count > 0)
		{
				pending_request->write_count--;
				QUEUE_appendNode(&pending_request_list,pending_request);
		}

		if (pending_request->write_count == 0)
		{
			if (pending_request->sync_write_response == false && pending_request->request_id == 0)
			{
				pthread_mutex_lock(&ppd_list_mutex);
				ppd_node_t *ppd = PPDLIST_getByFd(ppd_list,pending_request->pfs_fd);
				pthread_mutex_unlock(&ppd_list_mutex);

				pfs_request_t *new_pfsrequest = malloc(sizeof(pfs_request_t));

				msg[0] = WRITE_SECTORS;
				new_pfsrequest->request_id = 0;
				new_pfsrequest->msg = malloc(msg_len+3);
				memcpy(new_pfsrequest->msg,msg,msg_len+3);
				new_pfsrequest->pfs_fd = pending_request->pfs_fd;

				pthread_mutex_lock(&ppd->request_list_mutex);
				QUEUE_appendNode(&ppd->request_list,new_pfsrequest);
				sem_post(&ppd->request_list_sem);
				pthread_mutex_unlock(&ppd->request_list_mutex);
			}
			else if (pending_request->sync_write_response == true && pending_request->request_id == 0)
			{
				pending_writes_forSyncronization--;
				//print_Console("pending_writes_forSyncronization:",pending_writes_forSyncronization,1,true);

				if (pending_writes_forSyncronization == 0)
				{
					pthread_mutex_lock(&ppd_list_mutex);
					ppd_node_t *ppd = PPDLIST_getByStatus(ppd_list,SYNCHRONIZING);//Dame el que se termino de sincronizar recien
					ppd->status = READY;//Cambialo a ready
					pthread_mutex_unlock(&ppd_list_mutex);
					ACTIVE_DISKS_AMOUNT++;
					pthread_mutex_unlock(&sync_mutex);//Que se sincronize el proximo
					print_Console("FIN SINCRONZIACION:",ppd->disk_ID,1,true);
					PRAID_WRITE_LOG("FIN SINCRONZIACION");
					//HABILITO DISCO, TERMINO DE SINCRONIZAR
				}

			}
			else
			{
				pfs_node_t* pfs = PFSLIST_getByFd(pfs_list,pending_request->pfs_fd);


				 fd_set write_set;
				 FD_ZERO(&write_set);
				 FD_SET(pending_request->pfs_fd,&write_set);
				 select(pending_request->pfs_fd+1,NULL,&write_set,NULL,NULL);

				 pthread_mutex_lock(&pfs->sock_mutex);
				 int32_t sent = 0;

		 			if(FD_ISSET(pending_request->pfs_fd,&write_set))
 						sent = COMM_send(msg,pending_request->pfs_fd);//sendall? que pasa si el pfs se dio de baja?

				pthread_mutex_unlock(&pfs->sock_mutex);
			}
		}
		free(pending_request);
	}
	pthread_mutex_unlock(&pending_request_list_mutex);
}

void pfs_pending_request_addNew(uint32_t sector, uint32_t ppd_fd,uint32_t pfs_fd)
{
	pfs_pending_request_t *new_response = malloc(sizeof(pfs_pending_request_t));
	new_response->pfs_fd = pfs_fd;
	new_response->ppd_fd = ppd_fd;
	new_response->sector = sector;
	QUEUE_appendNode(&pending_request_list,new_response);
}


void pfs_pending_request_removeAll()
{
	queueNode_t *cur_ppd_node = ppd_list.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		queueNode_t* cur_request_node = cur_ppd->request_list.begin;

		while (cur_request_node != NULL)
		{
			free(((pfs_request_t*) cur_request_node->data)->msg);
			free(cur_request_node->data);
			queueNode_t* aux = cur_request_node;
			cur_request_node = cur_request_node->next;
			free(aux);

		}

		cur_ppd_node = cur_ppd_node->next;
	}
}

pfs_pending_request_t* pfs_pending_request_searchAndTake(queue_t* response_list,uint32_t request_id,uint32_t sector)
{
	queueNode_t *cur_node = response_list->begin;//RESPONSE LIST ES GLOBAL, SE PASA POR PARAMETRO?
	queueNode_t *prev_node = cur_node;
	while (cur_node != NULL)
	{
		pfs_pending_request_t *cur_response = (pfs_pending_request_t*) cur_node->data;

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

bool pfs_pending_request_exist(queue_t* response_list,uint32_t request_id,uint32_t sector)
{
	queueNode_t *cur_node = response_list->begin;

	while (cur_node != NULL)
	{
		pfs_pending_request_t *cur_response = (pfs_pending_request_t*) cur_node->data;

		if (cur_response->sector == sector && cur_response->request_id == request_id)
		{
			return true;
		}
		cur_node = cur_node->next;
	}
	return false;
}
