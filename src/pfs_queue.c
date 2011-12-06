/*
 * pfs_queue.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */


#include "pfs_queue.h"
#include "tad_queue.h"
#include <pthread.h>
#include <stdint.h>

queue_t PFS_QUEUE;

pthread_mutex_t* PFSQUEUE_getMutex(uint32_t pfs_fd)
{
	queueNode_t *pfs_node = PFS_QUEUE.begin;
	while (pfs_node != NULL)
	{
		pfs_node_t *pfs = (pfs_node_t*) pfs_node->data;
		if (pfs->pfs_fd == pfs_fd) return &pfs->socket_mutex;
		pfs_node = pfs_node->next;
	}
	return NULL;
}

void PFSQUEUE_addNew(uint32_t pfs_fd)
{
	pfs_node_t *new_pfs = malloc(sizeof(pfs_node_t));
	new_pfs->pfs_fd = pfs_fd;
	pthread_mutex_init(&new_pfs->socket_mutex,NULL);
	QUEUE_appendNode(&PFS_QUEUE,(void*) new_pfs);
}
