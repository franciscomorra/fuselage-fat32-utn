/*
 * pfs_queue.h
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */

#ifndef PFS_QUEUE_H_
#define PFS_QUEUE_H_

#include <stdint.h>
#include <pthread.h>
#include <stdint.h>


typedef struct pfs_node_t
{
	uint32_t pfs_fd;
	pthread_mutex_t socket_mutex;
} pfs_node_t;


pthread_mutex_t* PFSQUEUE_getMutex(uint32_t pfs_fd);

void PFSQUEUE_addNew(uint32_t pfs_fd);


#endif /* PFS_QUEUE_H_ */
