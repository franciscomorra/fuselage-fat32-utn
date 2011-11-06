#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include "comm.h"

char* COMM_recieve(uint32_t currFD,uint32_t* dataRecieved){

	char* msgHeader = malloc(3);								//alojo memoria para recibir la cabecera del mensaje (tipo y len)
	char* msgIn = NULL;

	if((*dataRecieved = recv(currFD,msgHeader,3,0)) != 0)		//recibo la cabecera y la guardo en msgHeader
	{
		uint16_t len = 0;
		memcpy(&len,msgHeader+1,2);								//copio el len en un int para poder usarlo
		msgIn = malloc(len+3);									//alojo memoria para recibir el payload del mensaje
		*dataRecieved += recv(currFD,msgIn+3,len,0);				//recibo el mensaje y lo guardo en msgIn +3
		memcpy(msgIn,msgHeader,3);								//concateno la cabecera con el payload del mensaje
	}

	free(msgHeader);
	return msgIn;
}

uint32_t COMM_send(char* msg,uint32_t fd)
{
	uint16_t len;
	uint32_t dataSent;
	memcpy(&len,msg+1,2);
	dataSent = send(fd,msg,len+3,0);

	return dataSent;
}
