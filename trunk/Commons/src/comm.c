#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include "nipc.h"
#include "comm.h"

char* COMM_receive(uint32_t currFD,uint32_t* dataReceived)
{

	char* msgHeader = malloc(3);								//alojo memoria para recibir la cabecera del mensaje (tipo y len)
	char* msgIn = NULL;

	if((*dataReceived = recv(currFD,msgHeader,3,0)) != 0)		//recibo la cabecera y la guardo en msgHeader
	{

		uint16_t len = 0;
		uint32_t left = 0;
		int32_t last_received = 0;
		memcpy(&len,msgHeader+1,2);
		left = len;//copio el len en un int para poder usarlo
		msgIn = malloc(len+3);
		while (left > 0)
		{//alojo memoria para recibir el payload del mensaje
			last_received = recv(currFD,msgIn+(*dataReceived),left,0);
			if (last_received > 0)
			{
				*dataReceived += last_received;
				left -= last_received;//recibo el mensaje y lo guardo en msgIn +3
			}
		}
		memcpy(msgIn,msgHeader,3);								//concateno la cabecera con el payload del mensaje
	}

	free(msgHeader);
	return msgIn;
}

uint32_t COMM_send(char* msg,uint32_t fd)
{
	uint16_t len = *((uint16_t*) (msg+1));
	uint32_t dataSent = 0;
	int32_t sent = 0;

	while (dataSent < len+3)
	{
		sent = send(fd,msg,len+3,MSG_DONTWAIT);
		if (sent > 0)
			dataSent += sent;
	}
	return dataSent;
}

char* COMM_receiveAll(uint32_t socket_fd,uint32_t* dataReceived,size_t *msg_len)
{
	char *msgIn = malloc(1);
	uint32_t one_byte = recv(socket_fd,msgIn,1,0);

	if (one_byte != 0)
	{
		if (msgIn[0] == -1) // ES UN MENSAJE DE AVISO
		{
			msgIn = realloc(msgIn,9);
			recv(socket_fd,msgIn+1,8,0);

			*msg_len = *((uint32_t*) (msgIn+5));
			int32_t left = *((int32_t*) (msgIn+1));
			free(msgIn);
			int32_t last_received = 0;
			char *all_msgIn = malloc(left);

			while (left > 0)
			{
				last_received = recv(socket_fd,all_msgIn+(*dataReceived),left,MSG_DONTWAIT);
				if (last_received >= 0)
				{
					*dataReceived += last_received;
					left -= last_received;
				}
			}

			return all_msgIn;
		}
		else if (msgIn[0] != WRITE_SECTORS && msgIn[0] != READ_SECTORS && msgIn[0] != HANDSHAKE && msgIn[0] != PPDCONSOLE_TRACE && msgIn[0] != PPDCONSOLE_INFO && msgIn[0] != PPDCONSOLE_EXIT)
		{
			printf("ERROR");

		}
		else
		{
			msgIn = realloc(msgIn,3);
			recv(socket_fd,msgIn+1,2,0);

			uint16_t len = *((uint16_t*) (msgIn+1));
			*msg_len = len+3;

			msgIn = realloc(msgIn,len+3);

			int32_t left = len;
			*dataReceived = 3;

			int32_t last_received = 0;

			while (left > 0)
			{
				last_received = recv(socket_fd,msgIn+(*dataReceived),left,MSG_DONTWAIT);
				if (last_received > 0)
				{
					*dataReceived += last_received;
					left -= last_received;
				}
				else
				{

				}
			}

			return msgIn;
		}
	}
	else
	{
		return NULL;
	}
}

void COMM_sendAdvise(uint32_t socket_descriptor,uint32_t data_size,uint32_t msg_size)
{
	char *msg = malloc(9);
	*msg = 0xFF;
	*((uint32_t*) (msg+1)) = data_size;
	*((uint32_t*) (msg+5)) = msg_size;
	send(socket_descriptor,msg,9,0);
	free(msg);
}

void COMM_sendHandshake(uint32_t fd,char* payload,uint32_t payload_len)
{
	char* handshake = malloc(3+payload_len);

	*handshake = HANDSHAKE;
	memcpy(handshake+1,&payload_len,2);

	if (payload != NULL)
		memcpy(handshake+3,payload,payload_len);

	send(fd,handshake,payload_len+3,NULL);
}

char* COMM_receiveHandshake(uint32_t fd,uint32_t* received)
{
	uint32_t len;
	return COMM_receiveAll(fd,received,&len);
}
