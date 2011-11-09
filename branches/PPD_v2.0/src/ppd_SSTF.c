
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>			//funcion sleep()
#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_taker.h"		//funcion getTimeSleep()
#include "tad_queue.h"


extern uint32_t headPosition;

uint32_t SSTF_getHead(queue_t* queue){			//TODO cambiar por getNext
	queueNode_t* aux = queue->begin;
	queueNode_t* prevAux = queue->begin;

	 while(aux != 0){
		 if(SSTF_near(((request_t*)aux->data)->CHS,((request_t*)queue->begin->data)->CHS)){
			 prevAux->next = aux->next;
			 aux->next = queue->begin;
			 queue->begin = aux;
		 }
		 prevAux = aux;
		 aux=aux->next;
	 }
	 return 1;
}


uint32_t SSTF_getNext(queue_t* queue, queueNode_t** prevCandidate){
	queueNode_t* currNode = queue->begin;
	CHS_t* headPCHS = COMMON_turnToCHS(headPosition);

	while(currNode->next != 0){
		if(*prevCandidate == NULL){				//si apunta a NULL quiere decir que el candidato a sacar es el primero de la cola
			if(TAKER_near(((request_t*)currNode->next->data)->CHS,headPCHS,((request_t*)queue->begin->data)->CHS)){
												//se fija si es preferible sacar el siguiente nodo al auxiliar que el primero de la cola
				*prevCandidate = currNode;		//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
				currNode = currNode->next;		//avanza el auxiliar
			} else
				currNode = currNode->next;		//si no es conveniente solo avanza el auxiliar
		} else {
			if(TAKER_near(((request_t*)currNode->next->data)->CHS,headPCHS,((request_t*)(*prevCandidate)->next->data)->CHS)){
												//se fija si es preferible sacar el siguiente nodo al auxiliar que el siguiente nodo al anterior al candidato
				*prevCandidate = currNode;		//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
				currNode = currNode->next;		//avanza el auxiliar
			} else
				currNode = currNode->next;		//si no es conveniente solo avanza el auxiliar
		}
	}

	return 1;		//Si devuelve NULL en prevCandidate quiere decir que hay q sacar el nodo que esta en begin de la queue
					//Si devuelve un nodo en prevCandidate, entonces el que habra que sacar es el siguiente.
}



uint32_t SSTF_near(CHS_t* new,CHS_t* queueHead){
	uint32_t i = TAKER_distanceTime(new);
	if(TAKER_distanceTime(new)< TAKER_distanceTime(queueHead))
		return 1;
	return 0;
}
