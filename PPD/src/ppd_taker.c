#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ppd_common.h"
#include "ppd_comm.h"
#include "nipc.h"
#include "ppd_io.h"
#include "ppd_taker.h"

extern uint32_t Sector;
extern uint32_t file_descriptor;
extern uint32_t bytes_perSector;
uint32_t headPosition;
uint32_t sectorNum;

void TAKER_handleRequest(requestNode_t* request){

	sectorNum = TAKER_turnToSectorNum(request);
	switch (request->type)
	{
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
	}
	headPosition = sectorNum+1;
}

uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode){
	uint32_t sectorNum;

	sectorNum = Sector*(CHSnode->cylinder + CHSnode->head)+CHSnode->sector;

	return sectorNum;
}
