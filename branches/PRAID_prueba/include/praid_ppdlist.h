/*
 * praid_ppdlist.h
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */

#ifndef PRAID_PPDLIST_H_
#define PRAID_PPDLIST_H_

#include <stdint.h>
#include "tad_queue.h"
#include <pthread.h>
#include <semaphore.h>
#include "nipc.h"

#define SYNCHRONIZING 1
#define WAIT_SYNCH 2
#define READY 3

typedef struct ppd_node
{
	pthread_t thread_id;
	uint32_t ppd_fd;
	queue_t request_list;
	pthread_mutex_t request_list_mutex;
	sem_t request_list_sem;
	uint32_t status;
} ppd_node_t;

typedef struct pfs_request
{
	uint32_t pfs_fd;
	nipcMsg_t msg;
} pfs_request_t;


void PPDLIST_addNewPPD(uint32_t ppd_fd,pthread_t thread_id);
void PFSREQUEST_addNew(uint32_t pfs_fd,char* msgFromPFS);
void PFSREQUEST_free(pfs_request_t *request);
ppd_node_t* PPDLIST_selectByLessRequests();

#endif /* PRAID_PPDLIST_H_ */