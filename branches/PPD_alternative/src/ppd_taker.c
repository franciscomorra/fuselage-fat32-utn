#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#include "ppd_common.h"
#include "ppd_comm.h"
#include "nipc.h"
#include "ppd_io.h"
#include "ppd_taker.h"
#include "tad_queue.h"
#include "ppd_qManager.h"
#include "ppd_SSTF.h"

extern uint32_t Sector;
extern uint32_t file_descriptor;
extern uint32_t bytes_perSector;
extern uint32_t TrackJumpTime;
extern uint32_t SectorJumpTime;
extern uint32_t NextDelay;
extern multiQueue_t* multiQueue;


uint32_t headPosition;
uint32_t sectorNum;

void TAKER_handleRequest(queue_t* queue, request_t* request,uint32_t delay,uint32_t(*getNext)(queue_t*,queueNode_t**)){
	sectorNum = TAKER_turnToSectorNum(request->CHS);
	CHS_t* headPosCHS = COMMON_turnToCHS(headPosition);
	uint32_t distance = TAKER_sectReachedDistance(request->CHS,headPosCHS);				//Distancia entre el pedido solicitado y la headPosition

	switch (request->type)
	{
		case PPDCONSOLE_TRACE:{
			//payload = headPosition+distance+sleep+(nextSector)+(cylinder-1 || 0)

			queueNode_t* queueNext =NULL;												//Siguiente pedido en la cola en CHS
			uint32_t nextSector;														//Siguiente pedido en la cola en Numero

			uint32_t len = sizeof(uint32_t)*4;											//Tamaño del payload
			request->payload = malloc(len);
			memset(request->payload,0,len);

			memcpy(request->payload,&headPosition,4);									//Si no hay proximo sector en la cola no copio nada al payload y disminuyo el Len
			TAKER_updateHPos(sectorNum);
			if(multiQueue->queueElemSem.__align != 0){
				QMANAGER_selectActiveQueue(multiQueue);
				NextDelay = getNext(queue,&queueNext);																						//Obtiene el proximo sector que mostrara segun la planificacion
				if(queueNext == NULL)
					nextSector = TAKER_turnToSectorNum(((request_t*)queue->begin->data)->CHS);
				else
					nextSector = TAKER_turnToSectorNum(((request_t*)queueNext->data)->CHS);
				memcpy(request->payload+12,&nextSector,4);
			} else len -= sizeof(uint32_t);

			memcpy(request->payload+4,&distance,4);
			memcpy(request->payload+8,&delay,4);
			memcpy(request->len,&len,2);												//actualiza el LEN del nodo
			break;
		}
		case READ_SECTORS:{
			request->payload = malloc(sizeof(char)*bytes_perSector);
			read_sector(file_descriptor,sectorNum,request->payload);
			memcpy(request->len, &bytes_perSector,2);
			TAKER_updateHPos(sectorNum);
			break;
		}
		case WRITE_SECTORS:{
			write_sector(file_descriptor, sectorNum, request->payload);
			TAKER_updateHPos(sectorNum);
			break;
		}
		default:
			break;
	}

	free(headPosCHS);
}

request_t* TAKER_takeRequest(queue_t* queue, queueNode_t* prevCandidate,uint32_t* delay){
	request_t* request;
	queueNode_t* candidate;
	if(prevCandidate == 0){
		request = queue->begin->data;
		candidate = queue->begin;
		queue->begin = queue->begin->next;
		if(queue->begin == NULL)
			queue->end = NULL;
	} else {
		request = prevCandidate->next->data;
		candidate = prevCandidate->next;
		prevCandidate->next = candidate->next;
		if(prevCandidate->next == NULL)
			queue->end = prevCandidate;
	}
	*delay += TAKER_distanceTime(request->CHS) + NextDelay;
	NextDelay = 0;
	//sleep(*delay/1000);
	free(candidate);
	return request;
}


void TAKER_getTraceInfo(CHS_t* CHSrequest,uint32_t* distance,uint32_t* delay){

	CHS_t* headPosCHS = COMMON_turnToCHS(headPosition);
	*distance = TAKER_sectReachedDistance(CHSrequest,headPosCHS);
	*delay = TAKER_distanceTime(CHSrequest);							//calcula el tiempo que va a tardar dicho pedido

	free(headPosCHS);

}

uint32_t TAKER_turnToSectorNum(CHS_t* CHS){
	uint32_t sectorNum;

	sectorNum = Sector*(CHS->cylinder + CHS->head)+CHS->sector;

	return sectorNum;
}

uint32_t TAKER_distanceTime(CHS_t* CHSrequest){

	CHS_t*  headPosCHS = COMMON_turnToCHS(headPosition);

	uint32_t cTimeDistance = abs(headPosCHS->cylinder - CHSrequest->cylinder)*TrackJumpTime;
	uint32_t sTimeDistance = TAKER_sectReachedDistance(CHSrequest,headPosCHS)*SectorJumpTime;

	free(headPosCHS);
	return (cTimeDistance + sTimeDistance);
}

uint32_t TAKER_sectReachedDistance(CHS_t* CHSrequest,CHS_t* headPosCHS){

	uint32_t reachedSector = TAKER_reachedSector(CHSrequest->cylinder,headPosCHS);
	uint32_t sDistance = TAKER_sectorDist(reachedSector,CHSrequest->sector);

	return sDistance;
}

uint32_t TAKER_sectorDist(uint32_t fstSector, uint32_t lstSector){
	if (lstSector < fstSector)
		return (Sector - (fstSector - lstSector));
	else
		return (lstSector - fstSector);

	return 0;
}

void TAKER_updateHPos(uint32_t sectorNum){
	headPosition = sectorNum + 1;
	if((headPosition % Sector) == 0)
		headPosition = headPosition - Sector;
}

uint32_t TAKER_near(CHS_t* curr, CHS_t* headP,CHS_t* candidate,uint32_t (condition)(CHS_t,CHS_t)){

	//se fija si la distancia entre A y head Position es menor que la de head Position y B
	// si es asi devuelve 1
	// si la distancia entre cilindros es igual lo deja a modo FIFO
	switch (condition(*curr,*headP)+(condition(*candidate,*headP))*2){
		case 3:{
			uint32_t distTrackCurrH = abs(curr->cylinder - headP->cylinder);
			uint32_t distTrackCandH = abs(candidate->cylinder - headP->cylinder);
			if (distTrackCurrH < distTrackCandH)
				return 2;
			break;
		}
		case 1:{
			return 1;
			break;
		}
		default:{
			return 0;
			break;
		}
	}
	return 0;
}

uint32_t TAKER_getNextNode(queue_t* queue, queueNode_t** prevCandidate,conditionFunction_t condition){
	queueNode_t* currNode = queue->begin;
	CHS_t* headPCHS = COMMON_turnToCHS(headPosition);

	while(currNode->next != NULL){
			if(*prevCandidate == NULL){																											//si apunta a NULL quiere decir que el candidato a sacar es el primero de la cola
				if(TAKER_near(((request_t*)currNode->next->data)->CHS,headPCHS,((request_t*)queue->begin->data)->CHS,condition)>0){	//se fija si es preferible sacar el siguiente nodo al auxiliar que el primero de la cola
					*prevCandidate = currNode;																									//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
					currNode = currNode->next;																											//avanza el auxiliar
				} else
					currNode = currNode->next;																									//si no es conveniente solo avanza el auxiliar
			} else {
				if(TAKER_near(((request_t*)currNode->next->data)->CHS,headPCHS,((request_t*)(*prevCandidate)->next->data)->CHS,condition)>0){	//se fija si es preferible sacar el siguiente nodo al auxiliar que el siguiente nodo al anterior al candidato
					*prevCandidate = currNode;																											//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
					currNode = currNode->next;																										//avanza el auxiliar
				} else
					currNode = currNode->next;																											//si no es conveniente solo avanza el auxiliar
		}
	}
	if((*prevCandidate == NULL) && ((condition(*((request_t*)queue->begin->data)->CHS,*headPCHS)) == 0))
		return 0;


	free(headPCHS);
	return 1;	//Si devuelve NULL en prevCandidate quiere decir que hay q sacar el nodo que esta en begin de la queue
					//Si devuelve un nodo en prevCandidate, entonces el que habra que sacar es el siguiente.
}


uint32_t TAKER_reachedSector(uint32_t cylinder,CHS_t* headPosCHS){
	uint32_t cDistance = abs(cylinder - headPosCHS->cylinder);
	uint32_t reachedSector;
	if(cDistance == 0)
		reachedSector = headPosCHS->sector;
	else
		reachedSector = ((cDistance*(TrackJumpTime/SectorJumpTime))%Sector + headPosCHS->sector)%Sector;	//TODO cambiar a coma flotante

	return reachedSector;
}

