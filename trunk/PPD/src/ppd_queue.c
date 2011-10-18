
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "ppd_common.h"
#include "nipc.h"
#include "ppd_queue.h"

uint32_t QUEUE_add(nipcMsg_t msg, queue_t* queue){
	NIPC_node* new = malloc(sizeof(NIPC_node));
	new->msg = msg;
	new->next = 0;

	if(queue->head == 0)
		queue->head = new;
	else
		queue->tail->next = new;

	queue->tail = new;
	sem_post(&queue->sem);
	return 0;
}

requestNode_t* QUEUE_take(queue_t* queue){

	sem_wait(&queue->sem);
	NIPC_node* aux = queue->head;

	requestNode_t* new = COMMON_turnToCHS(queue->head->msg.payload);
	memcpy(&(new->type),&queue->head->msg.type,sizeof(NIPC_type));
	memcpy(new->payload,(queue->head->msg.payload)+4,(queue->head->msg.len)-4);
	//TODO sender cuando usemos sockets

	queue->head = queue->head->next;

	if(queue->head == 0)
		queue->tail = 0;

	free(aux);
	return new;
}
