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
extern requestNode_t* first;
extern uint32_t file_descriptor;
extern uint32_t bytes_perSector;
nipcMsg_t msg;
uint32_t headPosition;
uint32_t sectorNum;

uint32_t TAKER_main() {

	while(1){
		if(first != 0){
			requestNode_t* aux = first;

			TAKER_getRequest(first,msg);
			ppd_send(msg);
			first = first->next;
			free(aux);
		}
	}
	return 0;
}

uint32_t TAKER_getRequest(requestNode_t* first,nipcMsg_t msg){

	sectorNum = TAKER_turnToSectorNum(first);
	switch (first->type)
	{
	case READ:{
		char* buf = malloc((sizeof(char)*bytes_perSector) + sizeof(uint32_t));

		memcpy(buf,&sectorNum,4);
		read_sector(file_descriptor,sectorNum,buf+4);
		msg = NIPC_createMsg(first->type,bytes_perSector + 4,buf);
		break;
		}
	case WRITE:{
		write_sector(file_descriptor, sectorNum, first->payload);
		msg = NIPC_createMsg(first->type,4,(char*)sectorNum);
		break;
	 	}
	}
	headPosition = sectorNum;
	return 0;
}


uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode){
	uint32_t sectorNum;

	sectorNum = Sector*(CHSnode->cylinder + CHSnode->head)+CHSnode->sector;

	return sectorNum;
}
