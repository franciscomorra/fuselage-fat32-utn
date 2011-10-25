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
#include "nipc.h"


typedef struct praid_list_node{
	struct praid_list_node_content *info;
	struct praid_list_node *next;
}praid_list_node;

typedef struct praid_list_node_content{
	uint32_t tid;
	uint32_t ppdStatus;
	struct queue_t *colaSublista;
	//SOCKET PPD
}praid_list_node_content;

typedef struct praid_sublist_node_content{
	nipcMsg_t msg;
	uint32_t synch;//0=False - 1=True
	//SOCKET PFS
	struct praid_sublist_node_content *next;
}ppd_sublist_node;

praid_list_node* PRAID_list_appendNode(uint32_t tid);




#endif /* PRAID_QUEUE_H_ */
