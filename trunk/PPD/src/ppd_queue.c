
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "ppd_common.h"
#include "nipc.h"
#include "ppd_queue.h"

uint32_t QUEUE_add(nipcMsg_t msg, queue_t* queue){
	NIPC_node* new = malloc(sizeof(NIPC_node)); //convierte el nipcMsg_t a un NIPC_node
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

	sem_wait(&queue->sem); //se utiliza para que el programa entre en una espera activa si es que no hay elementos en la queue
	NIPC_node* aux = queue->head;

	requestNode_t* new = COMMON_turnToCHS(aux->msg.payload); //aca se crea el malloc del requestNode_t new
	new->type = aux->msg.type; //convertimos el contenido del NIPC_node a requestNode_t
	new->len = aux->msg.len;
	memcpy(new->payload,aux->msg.payload+4,aux->msg.len-4);
	//TODO sender cuando usemos sockets

	queue->head = aux->next;

	if(queue->head == 0)
		queue->tail = 0;

	free(aux);
	return new;
}
