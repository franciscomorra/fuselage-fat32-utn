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
#include <stdbool.h>

typedef enum {
	READY=0, SYNCHRONIZING=1, WAIT_SYNCH=2, DISCONNECTED=3
} PRAID_PPDthread_status;

typedef struct praid_list_node{
	pthread_t tid;
	PRAID_PPDthread_status ppdStatus;
	uint32_t socketPPD;
	struct queue_t* colaSublista;
	struct praid_list_node* next;
}praid_list_node;

typedef enum {
	UNREAD=0, SENT=1, RECEIVED=2
} PRAID_request_status;

typedef struct praid_sl_content{
	bool synch;
	nipcMsg_t msg;
	uint32_t socketPFS;
	PRAID_request_status status;
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
bool PRAID_hay_discos_sincronizandose(void);
uint32_t PRAID_Start_Synch(void);
uint32_t PRAID_actualizar_CurrentRead(void);
praid_list_node* PRAID_SearchPPDBySocket(uint32_t);
queueNode_t* PRAID_Search_WRITE_Queue(uint32_t);
queueNode_t* PRAID_Search_Requests_SL(uint32_t,queue_t*);
uint32_t NIPC_getID(nipcMsg_t);


#endif /* PRAID_QUEUE_H_ */
