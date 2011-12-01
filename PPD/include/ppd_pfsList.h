#ifndef PPD_PFSLIST_H_
#define PPD_PFSLIST_H_

#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>

#include "tad_queue.h"

typedef struct pfs_node {
	uint32_t sock_fd;
	pthread_mutex_t sock_mutex;
}pfs_node_t;

void PFSLIST_addNew(queue_t* pfsList,uint32_t fd);

pfs_node_t* PFSLIST_getByFd(queue_t pfsList,uint32_t fd);

void PFSLIST_destroyNode(pfs_node_t* candidateData,queue_t pfsList);

#endif /* PPD_PFSLIST_H_ */
