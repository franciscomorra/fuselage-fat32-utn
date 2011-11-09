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
	pthread_t tid;
	uint32_t ppdStatus; //0=Ready, 1=Sincronizando, 2=WaitSynch
	struct queue_t* colaSublista;
	//SOCKET PPD
}praid_list_node_content;

typedef struct praid_sl_content{
	uint32_t synch;//0=False - 1=True
	nipcMsg_t msg;
	uint32_t socket;
	//SOCKET PFS
}praid_sl_content;

praid_list_node* PRAID_list_appendNode(pthread_t);
uint32_t PRAID_add_READ_Request(praid_sl_content*);
uint32_t PRAID_add_WRITE_Request(praid_sl_content*);
uint32_t PRAID_clear_list_node(praid_list_node*);
uint32_t PRAID_ppd_thread_count();
uint32_t PRAID_hay_discos_sincronizandose();
uint32_t PRAID_Start_Synch();
uint32_t PRAID_actualizar_CurrentRead();



#endif /* PRAID_QUEUE_H_ */
