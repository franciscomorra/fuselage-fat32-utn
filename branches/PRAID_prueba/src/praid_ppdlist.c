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

void PPDLIST_addNewPPD(uint32_t ppd_fd,pthread_t thread_id)
{

	ppd_node_t *new_ppd = malloc(sizeof(ppd_node_t));
	new_ppd->ppd_fd = ppd_fd;
	new_ppd->thread_id = thread_id;
	if (QUEUE_length(&ppd_list) == 0)
	{
		new_ppd->status=READY;
	}
	else
	{
		new_ppd->status=WAIT_SYNCH;
	}

	QUEUE_initialize(&new_ppd->request_list);
	pthread_mutex_init(&new_ppd->request_list_mutex,NULL);
	sem_init(&new_ppd->request_list_sem,NULL,0);
	QUEUE_appendNode(&ppdlist,new_ppd);

}

void PFSREQUEST_addNew(uint32_t pfs_fd,char* msgFromPFS)
{
	nipcMsg_t new_request_msg = NIPC_toMsg(msgFromPFS);


	ppd_node_t *selected_ppd = PPDLIST_selectByLessRequests();
	pthread_mutex_lock(&selected_ppd->request_list_mutex);
	pfs_request_t *new_pfsrequest = malloc(sizeof(pfs_request_t));
	new_pfsrequest->msg = new_request_msg;
	new_pfsrequest->pfs_fd = pfs_fd;
	QUEUE_appendNode(&selected_ppd->request_list,new_pfsrequest);
	sem_post(&selected_ppd->request_list_sem);
	pthread_mutex_unlock(&selected_ppd->request_list_mutex);
	return;
}

void PFSREQUEST_free(pfs_request_t *request)
{
	free(request->msg.payload);
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

		size_t requests_number = QUEUE_length(&cur_ppd->request_list);
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
