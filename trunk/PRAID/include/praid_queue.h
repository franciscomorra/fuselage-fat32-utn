/*
 * praid_queue.h
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#ifndef PRAID_QUEUE_H_
#define PRAID_QUEUE_H_

#include <pthread.h>
#include "nipc.h"
#include "tad_queue.h"
#include <stdbool.h>
#include <semaphore.h>

typedef enum {
	READY=0, SYNCHRONIZING=1, WAIT_SYNCH=2, DISCONNECTED=3
} PRAID_PPDthread_status;

typedef struct praid_list_node{
	pthread_t tid;
	PRAID_PPDthread_status ppdStatus;
	uint32_t socketPPD;
	struct queue_t* colaSublista;
	struct praid_list_node* next;
	uint32_t diskID;
	uint32_t ammount_synch;
	sem_t request_list_sem;
}praid_list_node;

typedef enum {
	UNREAD=0, SENT=1, RECEIVED=2
} PRAID_request_status;

typedef struct praid_sl_content{
	bool synch;
	nipcMsg_t msg;
	uint32_t socketRequest;
	PRAID_request_status status;
}praid_sl_content;

typedef struct praid_write_content{
	uint32_t threads_left;
	uint32_t IDrequest;
}praid_write_content;

typedef struct praid_ppdThreadParam{
	uint32_t socketPPD;
	uint32_t diskID;
}praid_ppdThreadParam;



praid_list_node* PRAID_ADD_PPD_NODE(pthread_t, praid_ppdThreadParam*);
uint32_t PRAID_ADD_READ(praid_sl_content*);
uint32_t PRAID_ADD_WRITE(praid_sl_content*);
uint32_t PRAID_REMOVE_PPD(praid_list_node*);
uint32_t PRAID_ACTIVE_PPD_COUNT(void);
//bool PRAID_hay_discos_sincronizandose(void);
bool PRAID_DISK_ID_EXISTS(uint32_t);
uint32_t PRAID_START_SYNCHR(uint32_t);
uint32_t PRAID_REFRESH_CURRENT_READ(void);
praid_list_node* PRAID_GET_PPD_FROM_FD(uint32_t);
praid_list_node* PRAID_GET_SYNCH_PPD(void);
queueNode_t* PRAID_GET_WRITE_NODE_BY_ID(uint32_t);
queueNode_t* PRAID_GET_REQUEST_BY_ID(uint32_t,queue_t*);
//uint32_t NIPC_getID(nipcMsg_t);


#endif /* PRAID_QUEUE_H_ */
