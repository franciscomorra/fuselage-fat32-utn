#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include "comm.h"

char* COMM_receive(uint32_t currFD,uint32_t* dataReceived){

	char* msgHeader = malloc(3);								//alojo memoria para recibir la cabecera del mensaje (tipo y len)
	char* msgIn = NULL;

	if((*dataReceived = recv(currFD,msgHeader,3,0)) != 0)		//recibo la cabecera y la guardo en msgHeader
	{
		uint16_t len = 0;
		memcpy(&len,msgHeader+1,2);								//copio el len en un int para poder usarlo
		msgIn = malloc(len+3);									//alojo memoria para recibir el payload del mensaje
		*dataReceived += recv(currFD,msgIn+3,len,0);				//recibo el mensaje y lo guardo en msgIn +3
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

char* COMM_receiveWithAdvise(uint32_t socket_fd,uint32_t* dataRecieved,size_t *msg_len)
{
	char *msg_len_buf = malloc(9);
	uint32_t advise_length = recv(socket_fd,msg_len_buf,9,0);
	char *msgIn;

	if (advise_length != 0)
	{
		if (msg_len_buf[0] == -1)
		{
			msgIn = malloc(*((uint32_t*) (msg_len_buf+1)));
			*msg_len = *((uint32_t*) (msg_len_buf+5));

			int32_t left = 0;
			left = *((uint32_t*) (msg_len_buf+1));
			uint32_t received = 0;
			uint32_t total = 0;

			while (left > 0)
			{
				received = recv(socket_fd,msgIn+total,left,0);
				total += received;
				left -= received;
			}
			*dataRecieved = total;
			return msgIn;
		}
		free(msg_len_buf);
	}
	else
	{
		free(msg_len_buf);
		return NULL;
	}
}
