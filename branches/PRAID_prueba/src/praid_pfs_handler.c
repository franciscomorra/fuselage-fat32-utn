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

extern queue_t ppdlist;
extern pthread_mutex_t ppdlist_mutex;
extern queue_t pfslist;
extern queue_t responselist;
extern pthread_mutex_t responselist_mutex;
extern uint32_t sync_write_count;
extern sem_t sync_ready_sem;
extern pthread_mutex_t prueba_mutex;

void* PFSHANDLER_sendResponse(uint32_t ppd_fd,char* msg)
{
	uint32_t sector = *((uint32_t*) (msg+7));
	uint16_t msg_len = *((uint16_t*) (msg+1));
	uint32_t request_id = *((uint32_t*) (msg+3));
	queueNode_t *cur_response_node =	responselist.begin;
	queueNode_t *next_response_node = cur_response_node->next;

	pfs_response_t *first_response = (pfs_response_t*) cur_response_node->data;
	if (first_response->request_id == request_id && first_response->sector == sector)
	{
		if (first_response->write_count > 0)
			{first_response->write_count--;}

		if (first_response->write_count == 0)
		{
			if (first_response->sync_write_response == false && first_response->request_id == 0)
			{
				msg[0] = WRITE_SECTORS;
				first_response->sync_write_response = true;
				//printf("W%d -> PPD: %d\n",sector,first_response->pfs_fd);
				pthread_mutex_lock(&ppdlist_mutex);
				ppd_node_t *ppd = PPDLIST_getByFd(ppdlist,first_response->pfs_fd);
				pthread_mutex_unlock(&ppdlist_mutex);
				pthread_mutex_lock(&ppd->sock_mutex);
				send(first_response->pfs_fd,msg,(msg_len+3),NULL);
				pthread_mutex_unlock(&ppd->sock_mutex);
				pthread_mutex_unlock(&prueba_mutex);
			}
			else if (first_response->sync_write_response == true && first_response->request_id == 0)
			{
				sync_write_count--;
				if (sync_write_count == 0)
				{
					sem_post(&sync_ready_sem);
				}
			}
			else
			{
				pfs_node_t* pfs = PFSLIST_getByFd(pfslist,first_response->pfs_fd);
				pthread_mutex_lock(&pfs->sock_mutex);
				if (send(first_response->pfs_fd,msg,(msg_len+3),NULL) != (msg_len+3))
				{
						uint32_t santi = 2;	//ERROR AL ENVIAR
				}
				pthread_mutex_unlock(&pfs->sock_mutex);
				responselist.begin = next_response_node;
				if (responselist.begin == NULL) responselist.end = NULL;
				free(cur_response_node->data);
				free(cur_response_node);
			}
		}

	}
	else
	{
		while (next_response_node != NULL)
		{
			pfs_response_t *next_response = (pfs_response_t*) next_response_node->data;
			if (next_response->request_id == request_id && next_response->sector == sector)
			{
				if (next_response->write_count > 0) {
					next_response->write_count--;
				}

				if (next_response->write_count == 0)
				{
					if (next_response->sync_write_response == false && next_response->request_id == 0)
					{
						msg[0] = WRITE_SECTORS;
						next_response->sync_write_response = true;
						pthread_mutex_lock(&ppdlist_mutex);
						ppd_node_t *ppd = PPDLIST_getByFd(ppdlist,next_response->pfs_fd);
						pthread_mutex_unlock(&ppdlist_mutex);
						pthread_mutex_lock(&ppd->sock_mutex);
						send(next_response->pfs_fd,msg,(msg_len+3),NULL);
						pthread_mutex_unlock(&ppd->sock_mutex);
						pthread_mutex_unlock(&prueba_mutex);

						//printf("W%d -> PPD: %d\n",sector,next_response->pfs_fd);
					}
					else if (next_response->sync_write_response == true && next_response->request_id == 0)
					{
						sync_write_count--;
						if (sync_write_count == 0)
						{
							sem_post(&sync_ready_sem);
						}
					}
					else
					{
						pfs_node_t* pfs = PFSLIST_getByFd(pfslist,first_response->pfs_fd);
						pthread_mutex_lock(&pfs->sock_mutex);
						if (send(next_response->pfs_fd,msg,(msg_len+3),NULL) != (msg_len+3))
						{
							uint32_t santi = 2;
						}
						pthread_mutex_unlock(&pfs->sock_mutex);
						cur_response_node->next = next_response_node->next;
						if (cur_response_node->next == NULL) responselist.end = cur_response_node;
						free(next_response_node->data);
						free(next_response);
					}
				}
				break;

			}
			cur_response_node = next_response_node;
			next_response_node = next_response_node->next;
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
