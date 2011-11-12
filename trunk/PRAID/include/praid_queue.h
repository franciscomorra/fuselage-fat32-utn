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
#include "tad_queue.h"


typedef struct praid_list_node{
	struct praid_list_node_content *info;
	struct praid_list_node *next;
}praid_list_node;

typedef struct praid_list_node_content{
	pthread_t tid;
	uint32_t ppdStatus; //0=Ready, 1=Sincronizando, 2=WaitSynch
	struct queue_t* colaSublista;
	uint32_t socketPPD;
}praid_list_node_content;

typedef struct praid_sl_content{
	uint32_t synch;//0=False - 1=True
	nipcMsg_t msg;
	uint32_t socketPFS;
	uint32_t status;//0=Unread 1=Sent 2=Received
}praid_sl_content;

typedef struct praid_read_content{
	uint32_t threads_left;
	uint32_t IDrequest;
}praid_read_content;

praid_list_node* PRAID_list_appendNode(pthread_t, uint32_t);
uint32_t PRAID_ADD_READ(praid_sl_content*);
uint32_t PRAID_ADD_WRITE(praid_sl_content*);
uint32_t PRAID_clear_list_node(praid_list_node*);
uint32_t PRAID_ActiveThreads_Amount(void);
uint32_t PRAID_hay_discos_sincronizandose(void);
uint32_t PRAID_Start_Synch(void);
uint32_t PRAID_actualizar_CurrentRead(void);
praid_list_node* PRAID_Search_by_socket(uint32_t);
queueNode_t* PRAID_Search_WRITE(uint32_t);
queueNode_t* PRAID_SearchSL(uint32_t,queue_t*);


#endif /* PRAID_QUEUE_H_ */
