/*
 * praid_queue.h
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#ifndef PRAID_QUEUE_H_
#define PRAID_QUEUE_H_

#include <pthread.h>
#include "praid_comm.h"

typedef struct {
	nipcMsg_t msg;
	uint32_t threads_left;
	struct write_node* next;
} write_node;

typedef struct {
	nipcMsg_t msg;
	//TODO definir que se pone cuando es de PFS o PPD en sync.
	struct read_node* next;
} read_node;

//A SER DEPRECADO, USAR COMMONS



uint32_t praid_READ_add(nipcMsg_t msgIn);
uint32_t praid_WRITE_add(nipcMsg_t msgIn);
uint32_t praid_READ_remove();
uint32_t praid_WRITE_remove();
uint32_t praid_READ_status();
uint32_t praid_WRITE_status();
read_node praid_READ_first();
write_node praid_WRITE_first();

#endif /* PRAID_QUEUE_H_ */
