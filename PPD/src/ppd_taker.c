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


nipcMsg_t TAKER_handleRequest(requestNode_t* request){

	nipcMsg_t msg;
	sectorNum = TAKER_turnToSectorNum(request);
	switch (request->type)
	{
	case READ_SECTORS:{
		char* buf = malloc((sizeof(char)*bytes_perSector) + sizeof(uint32_t));

		memcpy(buf,&sectorNum,4);
		read_sector(file_descriptor,sectorNum,buf+4);
		msg = NIPC_createMsg(request->type,bytes_perSector + 4,buf);
		break;
		}
	case WRITE_SECTORS:{
		write_sector(file_descriptor, sectorNum, request->payload);
		msg = NIPC_createMsg(request->type,4,(char*)sectorNum);
		break;
	 	}
	}
	headPosition = sectorNum;
	return msg; //por alguna razon cuando queria que devuelva el msg por parametro me aparecia "out of bound" el payload
				// entonces tuve que hacer que lo devuela con el return
}


uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode){
	uint32_t sectorNum;

	sectorNum = Sector*(CHSnode->cylinder + CHSnode->head)+CHSnode->sector;

	return sectorNum;
}
