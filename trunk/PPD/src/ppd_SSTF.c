
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ppd_SSTF.h"
#include "ppd_common.h"
//#include "ppd_queue.h"
#include "tad_queue.h"

extern uint32_t TrackJumpTime;
extern uint32_t headPosition;

void SSTF_getHead(queue_t* queue){
	requestNode_t* CHSposition = malloc(sizeof(requestNode_t));
 	COMMON_turnToCHS(headPosition,CHSposition);
	queueNode_t* aux = queue->begin;
	queueNode_t* prevAux = queue->begin;

	 while(aux != 0){
		 if(SSTF_near(aux->data,CHSposition,queue->begin->data)){
			 prevAux->next = aux->next;
			 aux->next = queue->begin;
			 queue->begin = aux;
		 }
		 prevAux = aux;
		 aux=aux->next;
	 }
}


requestNode_t* SSTF_takeRequest(queue_t* queue){
	SSTF_getHead(queue);
	queueNode_t* node = (QUEUE_takeNode(queue));
	return node->data;
}
/*
uint32_t SSTF_addRequest(requestNode_t* new){

	 requestNode_t* CHSposition = COMMON_turnToCHS(headPosition);

	 if(first == 0){
		 sem_wait(&SSTFmutex); //semaforos que uso para que cuando accesa a first no pueda ser interrumpido por el TAKER
		 first = new;
		 sem_post(&SSTFmutex);
		 return 0;
	 } else {
		sem_wait(&SSTFmutex);
		requestNode_t* aux = first;
		if (SSTF_near(new,CHSposition,first)){
			new->next = first;
			first = new;
			sem_post(&SSTFmutex);
		} else {
			sem_post(&SSTFmutex);
			while (aux->next != 0){
				if(SSTF_near(new,aux,aux->next)){
					new->next = aux->next;
					aux->next = new;
					return 0;
				} else
					aux = aux->next;
			}
			aux->next = new;
		}

	}
	 return 0;
}
*/

uint32_t SSTF_near(requestNode_t* new, requestNode_t* aux,requestNode_t* auxSig){

	//se fija si la distancia entre  new y aux es menor que la de aux y auxSig
	// si es asi devuelve True

	uint32_t distTrackNA = abs(new->cylinder - aux->cylinder);
	uint32_t distTrackAS = abs(auxSig->cylinder - aux->cylinder);

	if (distTrackNA < distTrackAS)
		return 1;
	else
		if (distTrackNA == distTrackAS){
			if(SSTF_sectorDist(((aux->sector))+(distTrackNA*TrackJumpTime),new->sector)
			< SSTF_sectorDist(((aux->sector))+(distTrackNA*TrackJumpTime),auxSig->sector))
				return 1;
	}
	return 0;
}

uint32_t SSTF_sectorDist(uint32_t fstSector, uint32_t lstSector){
	if (lstSector < fstSector)
		return (Sector - (fstSector - lstSector));
	else
		return (lstSector - fstSector);

	return 0;
}

