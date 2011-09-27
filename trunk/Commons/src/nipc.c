/*
 * nipc.c
 *
 *  Created on: 27/09/2011
 *      Author: utn_so
 */

#include "nipc.h"

msgNIPC_t* NIPC_createMsg(char type,uint32_t len, char* payload)
{
	msgNIPC_t *msg = malloc(sizeof(msgNIPC_t));
	msg->type = type;
	memcpy(msg->len,&len,2);
	msg->payload = malloc(len);
	memcpy(msg->payload,payload,len);
	return msg;
}
