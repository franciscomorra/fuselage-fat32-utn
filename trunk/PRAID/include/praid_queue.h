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
//TODO Definir LISTA DE DISCOS Y SUBLISTAS
typedef struct {
	/*
	ppd_list_node_content* info;
	struct ppd_list_node* next;
	*/
} ppd_list_node;

typedef struct {
	uint32_t tid;
	uint32_t ppdStatus;
	//sublist_node sublist;
	//SOCKET PPD
} ppd_list_node_content;

typedef struct {
	nipcMsg_t msg;
	//SOCKET PFS
	uint32_t synch;//0=False - 1=True
} ppd_sublist_node_content;


#endif /* PRAID_QUEUE_H_ */
