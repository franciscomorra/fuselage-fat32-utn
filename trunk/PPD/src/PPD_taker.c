#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ppd_common.h"
#include "ppd_comm.h"
#include "ppd_taker.h"

extern uint32_t Sector;
extern requestNode_t* first;
uint32_t headPosition;
nipcMsg_t msg;

uint32_t TAKER_main() {

	while(1){
		if(first != 0){
			TAKER_getRequest(first,msg);
			ppd_send(msg);
		}
	}
	return 0;
}

uint32_t TAKER_getRequest(requestNode_t* first,nipcMsg_t msg){

	switch (first->type)
	{
	case READ:{

	}
		break;

	case WRITE:
		break;

	}
	return 0;
}


uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode){
	uint32_t sectorNum;

	sectorNum = Sector*(CHSnode->cylinder + CHSnode->head)+CHSnode->sector;

	return sectorNum;
}
