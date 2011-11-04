/*
 * nipc.c
 *
 *  Created on: 27/09/2011
 *      Author: utn_so
 */

#include "nipc.h"
#include <stdlib.h>
#include <string.h>


nipcMsg_t NIPC_createMsg(NIPC_type type,uint32_t payload_bytes_len, char* payload_bytes)
{
	nipcMsg_t msg;
	msg.type = type;
	memcpy(msg.len,&payload_bytes_len,2);
	msg.payload = malloc(payload_bytes_len);
	memset(msg.payload,0,payload_bytes_len);
	memcpy(msg.payload,payload_bytes,payload_bytes_len);
	return msg;
}

void NIPC_cleanMsg(nipcMsg_t* msg)
{
	free(msg->payload);
	return;
}


char* NIPC_toBytes(nipcMsg_t msg)
{
	uint32_t len = 0;
	memcpy(&len,msg.len,2);
	char *buf = malloc(3+len);

	memcpy(buf,&msg.type,1);
	memcpy(buf+1,msg.len,2);
	memcpy(buf+3,msg.payload,len);
	return buf;
}

nipcMsg_t NIPC_toMsg(char* msg)
{
	uint32_t len = 0;
	memcpy(&len,msg+1,2);
	nipcMsg_t nipc_msg;
	memcpy(&nipc_msg.type,msg,1);
	memcpy(nipc_msg.len,msg+1,2);
	nipc_msg.payload = malloc(len);
	memcpy(nipc_msg.payload,msg+3,len);

	return nipc_msg;
}



void NIPC_createCharMsg(char* msg,NIPC_type type,uint16_t payload_bytes_len,char* payload_bytes)
{
	//char* msg = malloc(3 + payload_bytes_len);
	*msg = type;
	memcpy(msg+1,&payload_bytes_len,2);
	memset(msg+3,0,payload_bytes_len);
	if (payload_bytes != NULL)
		memcpy(msg+3,payload_bytes,payload_bytes_len);
}

