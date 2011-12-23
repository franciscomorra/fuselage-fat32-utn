#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "ppd_common.h"
#include "ppd_translate.h"
#include "ppd_taker.h"


request_t* TRANSLATE_fromCharToRequest(char* msg,uint32_t sockFD)
{
	request_t* request = malloc(sizeof(request_t));

	memcpy(&request->ID,msg+3,4);

	uint32_t sectorNum;
	memcpy(&sectorNum,msg+7,4);
	CHS_t* CHSrequest = COMMON_turnToCHS(sectorNum);
	request->CHS = CHSrequest;

	request->type = msg[0];

	uint16_t len = 0;
	memcpy(&len,(msg+1),2);
	len = len - 8;
	memcpy(request->len,&len,2);

	request->sender = sockFD;
	request->payload = NULL;
	if(*msg == WRITE_SECTORS){
		request->payload = malloc(len);			//sirve para grabar en el payload la cantidad de datos que se quieren escribir en el disco
		memcpy(request->payload, msg+11,len);
	}
	return request;
}

char* TRANSLATE_fromRequestToChar(request_t* request)
{
	uint16_t len;
	memcpy(&len,request->len,2);
	char* msg = malloc(len + 11);
	msg[0] = request->type;
	uint32_t sectorNum = TAKER_turnToSectorNum(request->CHS);
	memcpy(msg+3,&request->ID,4);
	memcpy(msg+7,&sectorNum,4);
	memcpy(msg+11,request->payload,len);
	len += 8;
	memcpy(msg+1,&len,2);
	return msg;
}
