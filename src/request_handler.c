/*
 * request_handler.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */


#include "request_handler.h"
#include "tad_queue.h"
#include <stdint.h>

#include <stddef.h>

queue_t REQUEST_QUEUE;
pthread_mutex_t REQUEST_QUEUE_MUTEX;

void request_addNew(uint32_t ppd_fd,uint32_t pfs_fd,char* msg)
{
	request_t *new_request = malloc(sizeof(request_t));
	new_request->pfs_fd = pfs_fd;
	new_request->ppd_fd = ppd_fd;
	new_request->sector = *((uint32_t*)(msg+7));
	new_request->request_id = *((uint32_t*)(msg+3));
	new_request->msg = msg;
	new_request->msg_len = *((uint16_t*)(msg+1)) + 3;
	pthread_mutex_lock(&REQUEST_QUEUE_MUTEX);
	QUEUE_appendNode(&REQUEST_QUEUE,new_request);
	pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
}

request_t *request_take(uint32_t request_id,uint32_t sector)
{
	queueNode_t *cur_request_node = REQUEST_QUEUE.begin;//RESPONSE LIST ES GLOBAL, SE PASA POR PARAMETRO?
	queueNode_t *prev_request_node = cur_request_node;

	while (cur_request_node != NULL)
	{
		request_t *cur_request = (request_t*) cur_request_node->data;

		if (cur_request->sector == sector && cur_request->request_id == request_id)
		{
			if (prev_request_node == cur_request_node)
			{
				REQUEST_QUEUE.begin = cur_request_node->next;
				if (REQUEST_QUEUE.begin == NULL) REQUEST_QUEUE.end = NULL;
			}
			else
			{
				prev_request_node->next = cur_request_node->next;
				if (prev_request_node->next == NULL) REQUEST_QUEUE.end = prev_request_node;
			}
			free(cur_request_node);
			return cur_request;
		}
		prev_request_node = cur_request_node;
		cur_request_node = cur_request_node->next;
	}
	return NULL;
}

void request_free(request_t *request)
{
	free(request->msg);
	free(request);
}


