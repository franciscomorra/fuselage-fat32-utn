/*
 * praid_ppdlist.c
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */
#include "praid_ppdlist.h"
#include <pthread.h>
#include <stdint.h>
#include "tad_queue.h"
#include <semaphore.h>

extern pthread_mutex_t ppdlist_mutex;
extern queue_t ppdlist;


ppd_node_t *PPDLIST_addNewPPD(uint32_t ppd_fd,pthread_t thread_id)
{
	ppd_node_t *new_ppd = malloc(sizeof(ppd_node_t));
	new_ppd->ppd_fd = ppd_fd;
	new_ppd->thread_id = thread_id;
	if (QUEUE_length(&ppdlist) == 0)
	{
		new_ppd->status=READY;
	}
	else
	{
		new_ppd->status=WAIT_SYNCH;
	}

	QUEUE_initialize(&new_ppd->request_list);
	pthread_mutex_init(&new_ppd->request_list_mutex,NULL);
	pthread_mutex_init(&new_ppd->sock_mutex,NULL);
	sem_init(&new_ppd->request_list_sem,NULL,0);
	QUEUE_appendNode(&ppdlist,new_ppd);
	return new_ppd;

}

void PFSREQUEST_addNew(uint32_t pfs_fd,char* msgFromPFS)
{
	//nipcMsg_t new_request_msg = NIPC_toMsg(msgFromPFS);

	uint32_t msg_len = *((uint16_t*) (msgFromPFS+1)) + 3;
	char *msg = malloc(msg_len);
	memcpy(msg,msgFromPFS,msg_len);

	pfs_request_t *new_pfsrequest = malloc(sizeof(pfs_request_t));
	new_pfsrequest->request_id =  *((uint32_t*) (msg+3));
	new_pfsrequest->msg = msg;
	new_pfsrequest->pfs_fd = pfs_fd;

	queueNode_t *cur_ppd_node = ppdlist.begin;

	if (*msg == WRITE_SECTORS)
	{
		pthread_mutex_lock(&ppdlist_mutex);

		while (cur_ppd_node != NULL)
		{
			ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppd_node->data;
			if (cur_ppd->status == READY)
			{
				pthread_mutex_lock(&cur_ppd->request_list_mutex);
				QUEUE_appendNode(&cur_ppd->request_list,new_pfsrequest);
				sem_post(&cur_ppd->request_list_sem);
				pthread_mutex_unlock(&cur_ppd->request_list_mutex);
			}
			cur_ppd_node = cur_ppd_node->next;
		}
		pthread_mutex_unlock(&ppdlist_mutex);
	}
	else
	{
		ppd_node_t *selected_ppd = PPDLIST_selectByLessRequests();
		pthread_mutex_lock(&selected_ppd->request_list_mutex);
			QUEUE_appendNode(&selected_ppd->request_list,new_pfsrequest);
			sem_post(&selected_ppd->request_list_sem);
		pthread_mutex_unlock(&selected_ppd->request_list_mutex);
	}
	return;
}

void PFSREQUEST_free(pfs_request_t *request)
{
	free(request->msg);
	free(request);
	return;
}

ppd_node_t* PPDLIST_selectByLessRequests()
{
	pthread_mutex_lock(&ppdlist_mutex);
	queueNode_t *cur_ppdnode = ppdlist.begin;
	uint32_t less = 9999999;
	ppd_node_t *selected_one = NULL;
	while (cur_ppdnode != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppdnode->data;

		pthread_mutex_lock(&cur_ppd->request_list_mutex);
		size_t requests_number = QUEUE_length(&cur_ppd->request_list);
		pthread_mutex_unlock(&cur_ppd->request_list_mutex);
		if (requests_number < less && cur_ppd->status == READY)
		{
			selected_one = cur_ppd;
			less = requests_number;
		}

		cur_ppdnode = cur_ppdnode->next;
	}
	pthread_mutex_unlock(&ppdlist_mutex);
	return selected_one;
}

ppd_node_t* PPDLIST_getByFd(queue_t ppdlist,uint32_t fd)
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->ppd_fd == fd) return cur_ppd;
		cur_ppd_node = cur_ppd_node->next;
	}
}

void PPDLIST_reorganizeRequests(uint32_t ppd_fd)
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	queueNode_t *prev_ppd_node = NULL;
	pthread_mutex_lock(&ppdlist_mutex);
	while (cur_ppd_node != NULL)
	{
		if (((ppd_node_t*)cur_ppd_node->data)->ppd_fd == ppd_fd)
		{
			if (prev_ppd_node != NULL)
			{
				prev_ppd_node->next = cur_ppd_node->next;
				if (prev_ppd_node->next == NULL) ppdlist.end = prev_ppd_node;
			}
			else
			{
				ppdlist.begin = cur_ppd_node->next;
				if (ppdlist.begin == NULL)
				{
					ppdlist.end = NULL;
				}
			}
			break;
		}
		prev_ppd_node = cur_ppd_node;
		cur_ppd_node = cur_ppd_node->next;
	}
	pthread_mutex_unlock(&ppdlist_mutex);

			ppd_node_t *selected_ppd = (ppd_node_t*) cur_ppd_node->data;
			pthread_mutex_lock(&selected_ppd->request_list_mutex);
			queueNode_t *cur_request_node;
			while ((cur_request_node = QUEUE_takeNode(&selected_ppd->request_list)) != NULL)
			{
				pfs_request_t *cur_request = (pfs_request_t*) cur_request_node->data;
				if (*(cur_request->msg) != 0x02)
				{
					ppd_node_t *ppd = PPDLIST_selectByLessRequests();
					pthread_mutex_lock(&ppd->request_list_mutex);
					QUEUE_appendNode(&ppd->request_list,cur_request);
					sem_post(&ppd->request_list_sem);
					pthread_mutex_unlock(&ppd->request_list_mutex);
				}
				else
				{
					free(cur_request->msg);
					free(cur_request_node->data);
					free(cur_request_node);
				}
			}
			pthread_mutex_unlock(&selected_ppd->request_list_mutex);
			free(cur_ppd_node->data);
			free(cur_ppd_node);
}
