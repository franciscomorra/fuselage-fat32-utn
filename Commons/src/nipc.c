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

char* NIPC_createCharMsg(NIPC_type type,uint32_t payload_bytes_len,char* payload_bytes)
{
	char* msg = malloc(3 + payload_bytes_len);
	*msg = type;
	memcpy(msg+1,&payload_bytes_len,2);
	memset(msg+3,0,payload_bytes_len);
	if (payload_bytes != NULL)
		memcpy(msg+3,payload_bytes,payload_bytes_len);
	return msg;
}
