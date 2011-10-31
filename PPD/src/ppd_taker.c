#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

uint32_t headPosition;
uint32_t sectorNum;

void TAKER_handleRequest(queue_t* queue, requestNode_t* request){
	requestNode_t* CHSposition = malloc(sizeof(requestNode_t));
	COMMON_turnToCHS(headPosition,CHSposition);
	sectorNum = TAKER_turnToSectorNum(request);
	switch (request->type)
	{
		case PPDCONSOLE_TRACE:{
			uint32_t len = sizeof(uint32_t)*5;
			request->payload = malloc(len+3);									//12B para "headP","ProxSector" y "sectorPedido" + 3B tipo y len

			memcpy(request->payload+4,&headPosition,4);

			SSTF_getHead(queue);												//TODO cambiarlo cuando elejimos otro algoritmo
			uint32_t nextSector = TAKER_turnToSectorNum(queue->begin->data);
			memcpy(request->payload+8,&nextSector,4);

			uint32_t distance = TAKER_getReachedDistance(request,CHSposition);	//calcula la distancia entre el sector alcanzado luego de llegar al cilindro
			memcpy(request->payload+12,&distance,4);							//y el sector al cual se buscaba llegar

			uint32_t delay = TAKER_getSleepTime(request);						//calcula el tiempo que va a tardar dicho pedido
			memcpy(request->payload+16,&delay,4);

			memcpy(request->len,&len,2);										//actualiza el LEN del nodo
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
	free(CHSposition);
}

uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode){
	uint32_t sectorNum;

	sectorNum = Sector*(CHSnode->cylinder + CHSnode->head)+CHSnode->sector;

	return sectorNum;
}

uint32_t TAKER_getSleepTime(requestNode_t* request){
	requestNode_t* CHSposition = malloc(sizeof(requestNode_t));
	 COMMON_turnToCHS(headPosition,CHSposition);

	 uint32_t cDistance = abs(CHSposition->cylinder - request->cylinder)* TrackJumpTime;
	 uint32_t sDistance = TAKER_sectorDist(request->sector,CHSposition->sector)*SectorJumpTime;

	free(CHSposition);
	return (cDistance + sDistance);
}

uint32_t TAKER_getReachedDistance(requestNode_t* request,requestNode_t* CHSposition){
	uint32_t cDistance = abs(request->cylinder - CHSposition->cylinder);
	uint32_t reachedSector = cDistance*(TrackJumpTime/SectorJumpTime)+request->cylinder;
	return TAKER_sectorDist(reachedSector % Sector,CHSposition->sector);
}

uint32_t TAKER_sectorDist(uint32_t fstSector, uint32_t lstSector){
	if (lstSector < fstSector)
		return (Sector - (fstSector - lstSector));
	else
		return (lstSector - fstSector);

	return 0;
}
