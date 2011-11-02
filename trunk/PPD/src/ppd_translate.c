#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "ppd_common.h"
#include "ppd_translate.h"
#include "ppd_taker.h"


requestNode_t* TRANSLATE_fromCharToRequest(char* msg,uint32_t sockFD)
{
	requestNode_t* request = malloc(sizeof(requestNode_t));

	uint32_t sectorNum;
	memcpy(&sectorNum,msg+3,4);
	CHS_t* CHSrequest = COMMON_turnToCHS(sectorNum);
	request->CHS = CHSrequest;

	request->type = msg[0];

	uint32_t len = 0;
	memcpy(&len,(msg+1),2);
	len = len - 4;
	memcpy(request->len,&len,2);				//TODO mejorar para restar 4 al len del msg sin necesidad de variable de paso

	request->sender = sockFD;

	if(*msg == WRITE_SECTORS){
		request->payload = malloc(len);			//sirve para grabar en el payload la cantidad de datos que se quieren escribir en el disco
		memcpy(request->payload, msg+7,len); 	//TODO mejorar para no usar variable de paso
	}
	return request;
}

char* TRANSLATE_fromRequestToChar(requestNode_t* request)
{
	char* msg = malloc(((uint32_t)*request->len) + 7);
	msg[0] = request->type;
	uint32_t msgLen = (*request->len)+sizeof(uint32_t);
	memcpy(msg+1,&msgLen,2);
	uint32_t sectorNum = TAKER_turnToSectorNum(request->CHS);
	memcpy(msg+3,&sectorNum,4);
	memcpy(msg+7,request->payload,(uint32_t)*request->len);
	return msg;
}
