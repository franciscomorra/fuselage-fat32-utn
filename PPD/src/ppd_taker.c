#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>
#include <math.h>

#include "ppd_common.h"
#include "ppd_comm.h"
#include "comm.h"
#include "nipc.h"
#include "ppd_io.h"
#include "ppd_taker.h"
#include "tad_queue.h"
#include "ppd_qManager.h"
#include "ppd_SSTF.h"
#include "log.h"
#include "ppd_translate.h"
#include "ppd_pfsList.h"

extern uint32_t Sector;
extern uint32_t bytes_perSector;
extern uint32_t TrackJumpTime;
extern uint32_t TracePosition;
extern uint32_t SectorJumpTime;
extern multiQueue_t* multiQueue;
extern uint32_t HeadPosition;
extern sem_t queueAvailableMutex;
t_log* Log;
//extern sem_t mainMutex;
//extern queue_t pfsList;
//extern sem_t queueMutex;
//extern uint32_t file_descriptor;




void* TAKER_main(uint32_t(*getNext)(queue_t*,queueNode_t**,uint32_t))
{
	fd_set writeFDs;
	while(1){
		sem_wait(&multiQueue->queueElemSem);

		TracePosition = HeadPosition;
		request_t* request;
		queueNode_t* prevCandidate = NULL;

		queue_t* queue = QMANAGER_selectActiveQueue(multiQueue);
		uint32_t delay = getNext(queue,&prevCandidate,HeadPosition);
		request = TAKER_takeRequest(queue,prevCandidate,&delay);

		char* msg = TAKER_handleRequest(queue,request,delay,getNext,prevCandidate);
/*
		uint32_t a;											//
		if(request->type != PPDCONSOLE_TRACE)				//
			memcpy(&a,msg+7,4);								//
		else												//temporal, muestra los sectores atendidos
			memcpy(&a,msg+3,4);								//
		if(msg[0] == WRITE_SECTORS)
			printf("W");
		else
			printf("R");
		printf("%d\n",a);
		fflush(0);
*/
		int32_t sent = 0;
		FD_ZERO(&writeFDs);
		FD_SET(request->sender,&writeFDs);
		if(select(request->sender +1,NULL,&writeFDs,NULL,NULL) == -1)
			perror("select");
		while(sent == 0){
			if(FD_ISSET(request->sender,&writeFDs))
				sent = SOCKET_sendAll(request->sender,msg,*((uint16_t*)(msg+1)) + 3,0);
				//sent = COMM_send(msg,request->sender);
		}
		sem_post(&queueAvailableMutex);
		free(msg);
		free(request->payload);
		free(request->CHS);
		free(request);
	}
	return NULL;
}

char* TAKER_handleRequest(queue_t* queue, request_t* request,uint32_t delay,uint32_t(*getNext)(queue_t*,queueNode_t**,uint32_t),queueNode_t* prevCandidate){
	uint32_t sectorNum = TAKER_turnToSectorNum(request->CHS);
	char* msg;

//	CHS_t* tracePosCHS = COMMON_turnToCHS(TracePosition);
//	uint32_t distance = TAKER_sectReachedDistance(request->CHS,tracePosCHS);				//Distancia entre el pedido solicitado y la headPosition

	switch (request->type)
	{
		case PPDCONSOLE_TRACE:{
			msg = COMMON_writeInLog(queue,prevCandidate,request,sectorNum,getNext,delay);

			break;
		}
		case READ_SECTORS:{
			//char* logMsg;
			request->payload = malloc(sizeof(char)*bytes_perSector);
			IO_readDisk(sectorNum,request->payload);
			memcpy(request->len, &bytes_perSector,2);
			msg = TRANSLATE_fromRequestToChar(request);
			COMMON_writeInLog(queue,prevCandidate,request,sectorNum,getNext,delay);
			break;
		}
		case WRITE_SECTORS:{
			//char* logMsg;
			IO_writeDisk(sectorNum, request->payload);
			msg = TRANSLATE_fromRequestToChar(request);
			COMMON_writeInLog(queue,prevCandidate,request,sectorNum,getNext,delay);
			break;
		}
		default:
			break;
	}

	TAKER_updateHPos(sectorNum);
	return msg;
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
	*delay += TAKER_distanceTime(request->CHS);

	free(candidate);
	return request;
}


void TAKER_getTraceInfo(CHS_t* CHSrequest,uint32_t* distance,uint32_t* delay){

	CHS_t* headPosCHS = COMMON_turnToCHS(HeadPosition);
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

	CHS_t*  headPosCHS = COMMON_turnToCHS(TracePosition);

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
	HeadPosition = sectorNum + 1;
	if((HeadPosition % Sector) == 0)
		HeadPosition = HeadPosition - Sector;
}

uint32_t TAKER_near(request_t* curr, CHS_t* headP,request_t* candidate,uint32_t (condition)(CHS_t,CHS_t)){

	//se fija si la distancia entre A y head Position es menor que la de head Position y B
	// si es asi devuelve 1
	// si la distancia entre cilindros es igual lo deja a modo FIFO
	switch (condition(*curr->CHS,*headP)+(condition(*candidate->CHS,*headP))*2){
		case 3:{
			uint32_t distTrackCurrH = abs(curr->CHS->cylinder - headP->cylinder);
			uint32_t distTrackCandH = abs(candidate->CHS->cylinder - headP->cylinder);
			if (distTrackCurrH < distTrackCandH)
				return 2;
			else if (distTrackCurrH == distTrackCandH)
				if((curr->type == WRITE_SECTORS) && (candidate->type != WRITE_SECTORS))
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

uint32_t TAKER_getNextNode(queue_t* queue,uint32_t headPosition, queueNode_t** prevCandidate,conditionFunction_t condition){
	queueNode_t* currNode = queue->begin;
	CHS_t* headPCHS = COMMON_turnToCHS(headPosition);

	while(currNode->next != NULL){
			if(*prevCandidate == NULL){																					//si apunta a NULL quiere decir que el candidato a sacar es el primero de la cola
				if(TAKER_near((request_t*)currNode->next->data,headPCHS,(request_t*)queue->begin->data,condition)>0){	//se fija si es preferible sacar el siguiente nodo al auxiliar que el primero de la cola
					*prevCandidate = currNode;																			//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
					currNode = currNode->next;																			//avanza el auxiliar
				} else
					currNode = currNode->next;																			//si no es conveniente solo avanza el auxiliar
			} else {
				if(TAKER_near((request_t*)currNode->next->data,headPCHS,(request_t*)(*prevCandidate)->next->data,condition)>0){	//se fija si es preferible sacar el siguiente nodo al auxiliar que el siguiente nodo al anterior al candidato
					*prevCandidate = currNode;																					//si es conveniente sacarlo, le asigna al anterior al candidato el auxiliar
					currNode = currNode->next;																					//avanza el auxiliar
				} else
					currNode = currNode->next;																					//si no es conveniente solo avanza el auxiliar
		}
	}
	if((*prevCandidate == NULL) && ((condition(*((request_t*)queue->begin->data)->CHS,*headPCHS)) == 0)){
		free(headPCHS);
		return 0;
	}

	free(headPCHS);
	return 1;	//Si devuelve NULL en prevCandidate quiere decir que hay q sacar el nodo que esta en begin de la queue
					//Si devuelve un nodo en prevCandidate, entonces el que habra que sacar es el siguiente.
}


uint32_t TAKER_reachedSector(uint32_t cylinder,CHS_t* headPosCHS){
	uint32_t cDistance = abs(cylinder - headPosCHS->cylinder);
	uint32_t reachedSector;
	if(cDistance == 0)
		reachedSector = headPosCHS->sector;
	else{
		uint32_t timeReachedSector = (TrackJumpTime-1)/(SectorJumpTime)+1;
		reachedSector = ((cDistance*(uint32_t)timeReachedSector)%Sector + headPosCHS->sector)%Sector;
	}
	return reachedSector;
}
