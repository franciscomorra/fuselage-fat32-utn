/*
 * ppd_pfslist.c
 *
 *  Created on: 11/11/2011
 *      Author: utn_so
 */
#include "praid_pfslist.h"
#include "tad_queue.h"
#include <stdint.h>

pfs_node_t* PFSLIST_getByFd(queue_t pfslist,uint32_t fd)
{
	queueNode_t *cur_pfs_node = pfslist.begin;
	while (cur_pfs_node != NULL)
	{
		pfs_node_t* cur_pfs = (pfs_node_t*) cur_pfs_node->data;
		if (cur_pfs->sock_fd == fd)
			return cur_pfs;
		cur_pfs_node = cur_pfs_node->next;
	}
}

void PFSLIST_addNew(queue_t *pfslist,uint32_t fd)
{
	pfs_node_t* new_pfs = malloc(sizeof(pfs_node_t));
	new_pfs->sock_fd = fd;
	pthread_mutex_init(&new_pfs->sock_mutex,NULL);
	QUEUE_appendNode(pfslist,new_pfs);
}
