
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>			//funcion sleep()
#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_taker.h"		//funcion getTimeSleep()
#include "tad_queue.h"

uint32_t SSTF_getHead(queue_t* queue){			//TODO cambiar por getNext
	queueNode_t* aux = queue->begin;
	queueNode_t* prevAux = queue->begin;

	 while(aux != 0){
		 if(SSTF_near(((requestNode_t*)aux->data)->CHS,((requestNode_t*)queue->begin->data)->CHS)){
			 prevAux->next = aux->next;
			 aux->next = queue->begin;
			 queue->begin = aux;
		 }
		 prevAux = aux;
		 aux=aux->next;
	 }
	 return 1;
}
/*
uint32_t SSTF_getNext(queue_t* queue,requestNode_t* request){
	queueNode_t* currNode = queue->begin;
	queueNode_t* prevCandidate = 0;
	CHS_t* headPCHS = COMMON_turnToCHS(headPosition);

	while(currNode != 0){
		if(TAKER_near(,,,))

	}
}
*/
uint32_t SSTF_near(CHS_t* new,CHS_t* queueHead){
	uint32_t i = TAKER_distanceTime(new);
	if(TAKER_distanceTime(new)< TAKER_distanceTime(queueHead))
		return 1;
	return 0;
}
