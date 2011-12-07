/*
 * ppd_queue.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */

#include <pthread.h>
#include <stdint.h>

#include "ppd_queue.h"
#include "tad_queue.h"

queue_t PPD_QUEUE;
uint32_t PPD_COUNT = 0;
pthread_mutex_t PPD_QUEUE_MUTEX;

ppd_node_t* PPDQUEUE_addNewPPD(uint32_t ppd_fd,uint32_t disk_id,uint32_t sectors_count)
{
	ppd_node_t *new_ppd = malloc(sizeof(ppd_node_t));
	new_ppd->ppd_fd = ppd_fd;
	new_ppd->thread_id = 0;
	new_ppd->disk_id = disk_id;
	new_ppd->sectors_count = sectors_count;

	new_ppd->status = (PPD_COUNT == 0)? READY : WAIT_SYNCH;
	pthread_mutex_init(&new_ppd->sock_mutex,NULL);
//	QUEUE_initialize(&new_ppd->requests);
	pthread_mutex_lock(&PPD_QUEUE_MUTEX);
		QUEUE_appendNode(&PPD_QUEUE,new_ppd);
	pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
	PPD_COUNT++;
	return new_ppd;
}

ppd_node_t* PPDQUEUE_selectByLessRequests()//Recorre todos los pedidos (puede ser numero muy grande y consume tiempo)
{
	queueNode_t *cur_ppdnode = PPD_QUEUE.begin;
	uint32_t less = 9999999;
	ppd_node_t *selected_one = NULL;
	pthread_mutex_lock(&PPD_QUEUE_MUTEX);
	while (cur_ppdnode != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppdnode->data;

		if (cur_ppd->requests_count < less && cur_ppd->status == READY)
		{
			selected_one = cur_ppd;
			less = cur_ppd->requests_count;
		}

		cur_ppdnode = cur_ppdnode->next;
	}
	pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
	return selected_one;
}

ppd_node_t* PPDQUEUE_getByFd(uint32_t fd)
{
	queueNode_t *cur_ppd_node =PPD_QUEUE.begin;
	ppd_node_t *selected_ppd = NULL;
	pthread_mutex_lock(&PPD_QUEUE_MUTEX);
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->ppd_fd == fd)
		{
			selected_ppd = cur_ppd;
			break;
		}
		cur_ppd_node = cur_ppd_node->next;
	}
	pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
	return selected_ppd;
}


ppd_node_t* PPDQUEUE_getByID(uint32_t disk_id)
{
	queueNode_t *cur_ppd_node = PPD_QUEUE.begin;
	ppd_node_t *selected_ppd = NULL;
	pthread_mutex_lock(&PPD_QUEUE_MUTEX);
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->disk_id == disk_id)
		{
			selected_ppd = cur_ppd;
			break;
		}
		cur_ppd_node = cur_ppd_node->next;
	}
	pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
	return selected_ppd;
}

ppd_node_t* PPDQUEUE_getByStatus(uint32_t status)
{
	queueNode_t *cur_ppd_node = PPD_QUEUE.begin;
	ppd_node_t *selected_ppd = NULL;
	pthread_mutex_lock(&PPD_QUEUE_MUTEX);
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->status == status)
		{
			selected_ppd = cur_ppd;
			break;
		}
		cur_ppd_node = cur_ppd_node->next;
	}
	pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
	return selected_ppd;
}

uint32_t PPDQUEUE_removePPD(uint32_t disk_id)
{
	queueNode_t *cur_ppd_node = PPD_QUEUE.begin;//RESPONSE LIST ES GLOBAL, SE PASA POR PARAMETRO?
	queueNode_t *prev_ppd_node = cur_ppd_node;

	while (cur_ppd_node != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppd_node->data;

		if (cur_ppd->disk_id == disk_id)
		{
			if (prev_ppd_node == cur_ppd_node)
			{
				PPD_QUEUE.begin = cur_ppd_node->next;
				if (PPD_QUEUE.begin == NULL) PPD_QUEUE.end = NULL;
			}
			else
			{
				prev_ppd_node->next = cur_ppd_node->next;
				if (prev_ppd_node->next == NULL) PPD_QUEUE.end = prev_ppd_node;
			}
			free(cur_ppd_node);
			free(cur_ppd);
			return 0;
		}
		prev_ppd_node = cur_ppd_node;
		cur_ppd_node = cur_ppd_node->next;
	}
	return 1;
}
