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

	uint32_t a;
	memcpy(&a,msg+3,4);
	COMMON_turnToCHS(a,request);
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
	memcpy(msg+1,request->len,2);
	uint32_t sectorNum = TAKER_turnToSectorNum(request);
	memcpy(msg+3,&sectorNum,4);
	memcpy(msg+7,request->payload,*request->len);
	return msg;
}
