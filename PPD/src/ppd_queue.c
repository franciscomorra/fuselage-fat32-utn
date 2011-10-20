
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "ppd_common.h"
#include "nipc.h"
#include "ppd_queue.h"

extern uint32_t headPosition;
extern requestNode_t* first;

uint32_t QUEUE_add(nipcMsg_t msg, queue_t* queue){
	NIPC_node* new = malloc(sizeof(NIPC_node)); //convierte el nipcMsg_t a un NIPC_node
	new->msg = msg;
	new->next = 0;
	NIPC_node* aux = queue->tail;

	if(queue->head == 0)
		queue->head = new;
	else
		aux->next = new; // al igual que abajo en el take, no entiendo porque tengo que creear
						 // un aux para que no me tire error de que no reconoce el campo next
	queue->tail = new;
	sem_post(&queue->sem);
	return 0;
}

requestNode_t* QUEUE_take(queue_t* queue){

	sem_wait(&queue->sem); //se utiliza para que el programa entre en una espera activa si es que no hay elementos en la queue
	uint32_t sectorNum;

	if(first == 0)
		QUEUE_getHead(queue);

	NIPC_node* aux = queue->head;
	memcpy(&sectorNum,aux->msg.payload,4);
	requestNode_t* new = COMMON_turnToCHS(sectorNum); //aca se crea el malloc del requestNode_t new
	new->type = aux->msg.type;						  //convertimos el contenido del NIPC_node a requestNode_t
	memcpy(&new->len,aux->msg.len-4,2);				  //el len de request node es uint32_t para poder ser utilizado en el memcpy
	new->payload = malloc(new->len);
	memcpy(new->payload,(aux->msg.payload+4),new->len);
	//TODO sender cuando usemos sockets

	queue->head = aux->next;

	if(queue->head == 0)
		queue->tail = 0;

	free(aux);
	return new;
}

uint32_t QUEUE_getHead(queue_t* queue){
	NIPC_node* aux = queue->head;
	NIPC_node* prev = queue->head;
	NIPC_node* prevNewHead = queue->head;
	uint32_t diffHead = abs((uint32_t)*aux->msg.payload - headPosition);
	uint32_t diffAux;

	while(aux->next != 0){												//no me deja hacer el aux->next->msg.payload
		diffAux = abs((uint32_t)*aux->msg.payload - headPosition);		//entonces tuve que usar un prev
		if(diffHead > diffAux){
			prevNewHead = prev;
			diffHead = diffAux;
		}
		prev = aux;
		aux = aux->next;
	}
	prev->next = aux->next;
	aux->next = queue->head;
	queue->head = aux;

	return 0;
}
