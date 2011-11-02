#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "ppd_common.h"
#include "ppd_comm.h"
#include "nipc.h"
#include "ppd_io.h"
#include "ppd_taker.h"
#include "tad_queue.h"
#include "ppd_SSTF.h"

extern uint32_t Sector;
extern uint32_t file_descriptor;
extern uint32_t bytes_perSector;
extern uint32_t TrackJumpTime;
extern uint32_t SectorJumpTime;
extern sem_t queueElemSem;

uint32_t headPosition;
uint32_t sectorNum;

void TAKER_handleRequest(queue_t* queue, requestNode_t* request){
	sectorNum = TAKER_turnToSectorNum(request->CHS);
	switch (request->type)
	{
		case PPDCONSOLE_TRACE:{
			//payload = headPosition+distance+sleep+(queueHead)

			requestNode_t* queueHead =0;								//Siguiente pedido en la cola en CHS
			uint32_t nextSector;										//Siguiente pedido en la cola en Numero
			uint32_t distance;											//Distancia entre el pedido solicitado y la headPosition
			uint32_t delay;												//Tiempo que se tardara en leer el pedido solicitado
			uint32_t len = sizeof(uint32_t)*4;							//Tamaño del payload
			request->payload = malloc(len);
			memset(request->payload,0,len);
			TAKER_getTraceInfo(request->CHS,&distance,&delay);


			memcpy(request->payload,&headPosition,4);					//Si no hay proximo sector en la cola no copio nada al payload y disminuyo el Len
			if(queueElemSem.__align != 0){
				SSTF_getHead(queue);									//TODO cambiarlo cuando elejimos otro algoritmo
				queueHead = queue->begin->data;							//Obtiene el proximo sector que mostrara segun la planificacion
				nextSector = TAKER_turnToSectorNum(queueHead->CHS);
				memcpy(request->payload+12,&nextSector,4);
			} else len = len - sizeof(uint32_t);

			memcpy(request->payload+4,&distance,4);
			memcpy(request->payload+8,&delay,4);
			memcpy(request->len,&len,2);								//actualiza el LEN del nodo
			break;
		}
		case READ_SECTORS:{
			request->payload = malloc(sizeof(char)*bytes_perSector);
			read_sector(file_descriptor,sectorNum,request->payload);
			memcpy(request->len, &bytes_perSector,2);
			break;
		}
		case WRITE_SECTORS:{
			write_sector(file_descriptor, sectorNum, request->payload);
			break;
		}
		default:
			break;
	}
	headPosition = sectorNum+1;
}

void TAKER_getTraceInfo(CHS_t* CHSrequest,uint32_t* distance,uint32_t* delay){

	CHS_t* CHSposition = COMMON_turnToCHS(headPosition);
	*distance = TAKER_getReachedDistance(CHSrequest,CHSposition);
	*delay = TAKER_getSleepTime(CHSrequest);							//calcula el tiempo que va a tardar dicho pedido

	free(CHSposition);

}

uint32_t TAKER_turnToSectorNum(CHS_t* CHS){
	uint32_t sectorNum;

	sectorNum = Sector*(CHS->cylinder + CHS->head)+CHS->sector;

	return sectorNum;
}

uint32_t TAKER_getSleepTime(CHS_t* CHSrequest){
	CHS_t* CHSposition = malloc(sizeof(requestNode_t));
	 CHSposition = COMMON_turnToCHS(headPosition);

	 uint32_t cDistance = abs(CHSposition->cylinder - CHSrequest->cylinder)* TrackJumpTime;
	 uint32_t sDistance = TAKER_sectorDist(CHSrequest->sector,CHSposition->sector)*SectorJumpTime;

	free(CHSposition);
	return (cDistance + sDistance);
}

uint32_t TAKER_getReachedDistance(CHS_t* CHSrequest,CHS_t* CHSposition){
	uint32_t cDistance = abs(CHSrequest->cylinder - CHSposition->cylinder);
	uint32_t reachedSector;
	if(cDistance == 0)
		reachedSector = CHSposition->sector;
	else
	 reachedSector = (cDistance*(TrackJumpTime/SectorJumpTime)+CHSrequest->cylinder) % Sector;
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
