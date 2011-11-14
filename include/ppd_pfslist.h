/*
 * ppd_pfslist.h
 *
 *  Created on: 11/11/2011
 *      Author: utn_so
 */

#ifndef PPD_PFSLIST_H_
#define PPD_PFSLIST_H_
#include <pthread.h>
#include <stdint.h>
#include "tad_queue.h"

typedef struct pfs_node
{
	uint32_t sock_fd;
	pthread_mutex_t sock_mutex;
} pfs_node_t;

void PFSLIST_addNew(queue_t *pfslist,uint32_t fd);
pfs_node_t* PFSLIST_getByFd(queue_t pfslist,uint32_t fd);
#endif /* PPD_PFSLIST_H_ */
