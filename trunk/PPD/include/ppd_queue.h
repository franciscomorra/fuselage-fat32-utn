/*
 * ppd_queue.h
 *
 *  Created on: 17/10/2011
 *      Author: utn_so
 */

#ifndef PPD_QUEUE_H_
#define PPD_QUEUE_H_

#include <semaphore.h>
#include "nipc.h"
#include "ppd_common.h"


typedef struct {
	NIPC_node* head;
	NIPC_node* tail;
	sem_t sem;
}	 queue_t;

uint32_t QUEUE_add(nipcMsg_t msg, queue_t* buffer);

requestNode_t* QUEUE_take(queue_t* buffer);

uint32_t QUEUE_getHead(queue_t* queue);

#endif /* PPD_QUEUE_H_ */
