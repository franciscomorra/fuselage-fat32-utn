#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>

#include "ppd_taker.h"
#include "tad_queue.h"
#include "ppd_qManager.h"
#include "ppd_common.h"
#include "log.h"
#include "tad_sockets.h"
#include "config_manager.h"

extern uint32_t Cylinder;
extern uint32_t Head;
extern uint32_t Sector;
extern uint32_t HeadPosition;
extern uint32_t TracePosition;
extern uint32_t TrackJumpTime;
extern uint32_t ReadTime;
extern uint32_t WriteTime;
extern flag_t Algorithm;
extern e_message_level LogFlag;
extern multiQueue_t* multiQueue;
extern t_log* Log;
extern sem_t mainMutex;

CHS_t* COMMON_turnToCHS(uint32_t sectorNum){
	CHS_t* CHS = malloc(sizeof(CHS_t));

	CHS->cylinder = (sectorNum) / (Sector * Head);
	CHS->head = (sectorNum % (Sector * Head)) / Sector;
	CHS->sector = (sectorNum % (Sector * Head)) % Sector;
	return CHS;
}

uint32_t COMMON_identity(CHS_t A,CHS_t B){
	return 1;
}

uint32_t COMMON_lessThan(CHS_t A, CHS_t B){

	return(A.cylinder <= B.cylinder);
}

uint32_t COMMON_greaterThan(CHS_t A,CHS_t B){

	return(A.cylinder >= B.cylinder);
}

char* COMMON_createLogChar(uint32_t sectorNum,request_t* request,uint32_t delay,uint32_t(*getNext)(queue_t*,queueNode_t**,uint32_t)){
	//msg = type + len + sectorNum + headPosition + distance + delay + TracePosition + (nextSector)

	uint16_t len = 3 + 6*sizeof(uint32_t); 																//cantidad de bytes alojados para el msg
	char* msg = malloc(len);
	queueNode_t* queueNext = NULL;
	uint32_t nextSector;																	//siguiente pedido, lo elije no lo saca
	CHS_t* tracePosCHS = COMMON_turnToCHS(TracePosition);									//variable que determina si el algoritmo tuvo que irse a algun extremo
	uint32_t distance = TAKER_sectReachedDistance(request->CHS,tracePosCHS);				//Distancia entre el pedido solicitado y el extremo (si tuvo que cambiar de direccion, sino es igual al headP)

	memcpy(msg,&request->type,1);
	memcpy(msg+3,&sectorNum,4);
	memcpy(msg+7,&HeadPosition,4);
	memcpy(msg+11,&distance,4);
	memcpy(msg+15,&delay,4);
	memcpy(msg+19,&TracePosition,4);														//indica si se cambio de direccion para mostrar diferentes cosas en el trace o log

	if(multiQueue->queueElemSem.__align != 0){

		sem_wait(&mainMutex);
		flag_t previousDirection = multiQueue->direction;									//guardamos la direccion del cabezal por si la busqueda del proximo sector nos cambia la direccion del mismo
		flag_t previousFlag = multiQueue->qflag;
		queue_t* queue = QMANAGER_selectActiveQueue(multiQueue);
		getNext(queue,&queueNext,sectorNum + 1);
		multiQueue->direction = previousDirection;
		multiQueue->qflag = previousFlag;
		sem_post(&mainMutex);																//Obtiene el proximo sector que mostrara segun la planificacion

		if(queueNext == NULL)
			nextSector = TAKER_turnToSectorNum(((request_t*)queue->begin->data)->CHS);
		else
			nextSector = TAKER_turnToSectorNum(((request_t*)queueNext->next->data)->CHS);
		memcpy(msg+23,&nextSector,4);

	} else len -= sizeof(uint32_t);															//Si no hay proximo sector en la cola no copio nada al payload y disminuyo el Len
	memcpy(msg+1,&len,2);

	free(tracePosCHS);
	return msg;
}

void COMMON_activeQueueStatus(queue_t* queue,queueNode_t* prevCandidate,CHS_t* CHSrequest){
	queueNode_t* queueNode = queue->begin;
	fprintf(Log->file,"Cola de Pedidos Activa: [");

	if(queueNode == NULL){
		fprintf(Log->file,"(%d:%d:%d)]\n",CHSrequest->cylinder,CHSrequest->head,CHSrequest->sector);
		return;
	}

	if(prevCandidate == NULL){
		fprintf(Log->file,"(%d:%d:%d),",CHSrequest->cylinder,CHSrequest->head,CHSrequest->sector);
	}

	while(queueNode->next != NULL){
		fprintf(Log->file,"(%d:%d:%d),",((request_t*)queueNode->data)->CHS->cylinder,((request_t*)queueNode->data)->CHS->head,((request_t*)queueNode->data)->CHS->sector);
		if(queueNode == prevCandidate)
			fprintf(Log->file,"(%d:%d:%d),",CHSrequest->cylinder,CHSrequest->head,CHSrequest->sector);
		queueNode = queueNode->next;
	}
	if(queueNode == prevCandidate){
		fprintf(Log->file,"(%d:%d:%d),",((request_t*)queueNode->data)->CHS->cylinder,((request_t*)queueNode->data)->CHS->head,((request_t*)queueNode->data)->CHS->sector);
		fprintf(Log->file,"(%d:%d:%d)]\n",CHSrequest->cylinder,CHSrequest->head,CHSrequest->sector);
	} else {
		fprintf(Log->file,"(%d:%d:%d)]\n",((request_t*)queueNode->data)->CHS->cylinder,((request_t*)queueNode->data)->CHS->head,((request_t*)queueNode->data)->CHS->sector);
	}
}

void COMMON_passiveQueueStatus(){
	queueNode_t* queueNode;

	fprintf(Log->file,"Cola de Pedidos Pasiva: [");
	queueNode = (QMANAGER_selectPassiveQueue(multiQueue))->begin;
	if (queueNode == NULL)
		fprintf(Log->file,"]\n");
	else{
	while(queueNode->next != NULL){
		fprintf(Log->file,"(%d:%d:%d),",((request_t*)queueNode->data)->CHS->cylinder,((request_t*)queueNode->data)->CHS->head,((request_t*)queueNode->data)->CHS->sector);
		queueNode = queueNode->next;
	}
	fprintf(Log->file,"(%d:%d:%d)]\n",((request_t*)queueNode->data)->CHS->cylinder,((request_t*)queueNode->data)->CHS->head,((request_t*)queueNode->data)->CHS->sector);
	}
}

void COMMON_writeInLog(queue_t* queue,char* msg,queueNode_t* prevCandidate,CHS_t* CHSrequest){
	//pthread_mutex_lock(&Log->mutex);
	log_info(Log,"TAKER",NULL);
	if(Log->log_levels == INFO){
		COMMON_activeQueueStatus(queue,prevCandidate,CHSrequest);
		if(multiQueue->qflag != SSTF)
			COMMON_passiveQueueStatus();
		fprintf(Log->file,"Cantidad Total de Pedidos: %d\n",(uint32_t)(multiQueue->queueElemSem.__align)+1);
		log_showTrace(msg,Log->file,Sector,Head,Log);
	}
	//pthread_mutex_unlock(&Log->mutex);
}


void COMMON_readPPDConfig(uint32_t* RPM, uint32_t* port, uint32_t* diskID,uint32_t* startingMode, char** IP,
		char** sockUnixPath,char** diskFilePath,char** consolePath,char** logPath,flag_t* initialDirection){
	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));			//
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));					//
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));				//
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));	//	leer archivo de configuración
	HeadPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));	//
	*RPM = atoi(CONFIG_getValue(ppd_config,"RPM"));						//
	*port = atoi(CONFIG_getValue(ppd_config,"Port"));
	*diskID = atoi(CONFIG_getValue(ppd_config,"DiskID"));
	ReadTime = atoi(CONFIG_getValue(ppd_config,"ReadTime"));
	WriteTime = atoi(CONFIG_getValue(ppd_config,"WriteTime"));

	*IP = malloc(strlen(CONFIG_getValue(ppd_config,"IP")));
	strcpy(*IP,CONFIG_getValue(ppd_config,"IP"));

	*sockUnixPath = malloc(strlen(CONFIG_getValue(ppd_config,"SockUnixPath")));
	strcpy(*sockUnixPath,CONFIG_getValue(ppd_config,"SockUnixPath"));

	*diskFilePath = malloc(strlen(CONFIG_getValue(ppd_config,"DiskFilePath")));
	strcpy(*diskFilePath,CONFIG_getValue(ppd_config,"DiskFilePath"));

	*consolePath = malloc(strlen(CONFIG_getValue(ppd_config,"ConsolePath")));
	strcpy(*consolePath,CONFIG_getValue(ppd_config,"ConsolePath"));

	*logPath = malloc(strlen(CONFIG_getValue(ppd_config,"LogPath")));
	strcpy(*logPath,CONFIG_getValue(ppd_config,"LogPath"));

	if(strcmp("SSTF",CONFIG_getValue(ppd_config,"Algorithm")) == 0)		//
		Algorithm = SSTF;
	else
		Algorithm = FSCAN;
	if(strcmp("LISTEN",CONFIG_getValue(ppd_config,"StartingMode")) == 0)		//
		*startingMode = MODE_LISTEN;
	else
		*startingMode = MODE_CONNECT;

	if(strcmp("UP",CONFIG_getValue(ppd_config,"Direction")) == 0)
		*initialDirection = UP;
	else
		*initialDirection = DOWN;

	if(strcmp("INFO",CONFIG_getValue(ppd_config,"LogFlag"))==0)
		LogFlag = INFO;
	else
		if(strcmp("ERROR",CONFIG_getValue(ppd_config,"LogFlag"))==0)
			LogFlag = ERROR;
		else
			LogFlag = OFF;

	CONFIG_destroyList(ppd_config);

}
