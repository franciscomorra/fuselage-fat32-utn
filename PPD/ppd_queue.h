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

typedef struct {
	NIPC_node head;
	NIPC_node tail;
	sem_t sem;
} queue_t;

#endif /* PPD_QUEUE_H_ */
