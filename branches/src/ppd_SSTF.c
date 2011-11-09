
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>	//funcion sleep()

#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_taker.h"		//funcion getTimeSleep()
#include "tad_queue.h"

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

	uint32_t takerReturn = TAKER_getNextNode(queue,prevCandidate,COMMON_identity);
	return takerReturn;
}




uint32_t SSTF_near(CHS_t* new,CHS_t* queueHead){
	if(TAKER_distanceTime(new)< TAKER_distanceTime(queueHead))
		return 1;
	return 0;
}
