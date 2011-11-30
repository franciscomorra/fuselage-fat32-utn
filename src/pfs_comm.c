/*
 * pfs_comm.c
 *
 *  Created on: 08/09/2011
 *      Author: utn_so
 */

//PROBLEMAS A SOLUCIONAR: LINKEO CON COMMONS Y PASAJE DE LENGTH DE NIPC

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "pfs_comm.h"
#include "comm.h"

#include "tad_sector.h"
#include <stdbool.h>
#include <semaphore.h>
#include "tad_bootsector.h"
#include "tad_sockets.h"
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>


extern bootSector_t boot_sector;
extern socketPool_t sockets_toPPD;
extern uint32_t request_id;

char* PPDINTERFACE_readSectors(uint32_t* sectors_array, size_t sectors_array_len)
{
	/* BUSQUEDA DE UN SOCKET LIBRE */
	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = PPDINTERFACE_getFreeSocket();
	pthread_mutex_lock(&ppd_socket->sock_mutex);
	/* FIN BUSQUEDA DE SOCKET LIBRE */

	size_t sector_index = 0,responses_received = 0;
	size_t response_message_len = 523;
	char *buffer = malloc(response_message_len*sectors_array_len);

	for (;sector_index < sectors_array_len;sector_index++)
	{
		char *msg_buf;
		int32_t readable_bytes = 0;

		if (ioctl(ppd_socket->descriptor,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{
				msg_buf = malloc(response_message_len);
				int32_t received = SOCKET_recvAll(ppd_socket->descriptor,msg_buf,response_message_len,MSG_DONTWAIT);

				if (received > 0)
				{
					memcpy(buffer+(responses_received*response_message_len),msg_buf,response_message_len);
					responses_received++;
				}
				else if (received == SOCK_DISCONNECTED || received == SOCK_ERROR)
				{
					free(msg_buf);
					free(buffer);
					printf("ERROR: Se perdio la conexion con el otro extremo.\n");
					exit(-1);
				}
				free(msg_buf);
			}
		}

		msg_buf = malloc(11);

		*msg_buf = READ_SECTORS;
		*((uint16_t*) (msg_buf+1)) = 8;
		*((uint32_t*) (msg_buf+3)) = request_id;
		*((uint32_t*) (msg_buf+7)) = sectors_array[sector_index];

		int32_t sent = SOCKET_sendAll(ppd_socket->descriptor,msg_buf,11,NULL);
		if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
		{
			printf("ERROR: Se perdio la conexion con el otro extremo.\n");
			exit(-1);
		}
		free(msg_buf);
	}

	for (;responses_received<sectors_array_len;responses_received++)
	{
		char *msg_buf = malloc(response_message_len);
		int32_t received = SOCKET_recvAll(ppd_socket->descriptor,msg_buf,response_message_len,NULL);

		if (received > 0)
		{
			memcpy(buffer+(responses_received*response_message_len),msg_buf,response_message_len);
		}
		else if (received == SOCK_DISCONNECTED || received == SOCK_ERROR)
		{
			free(buffer);
			free(msg_buf);
			printf("ERROR: Se perdio la conexion con el otro extremo.\n");
			exit(-1);
		}
		free(msg_buf);
	}

	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);
	char *final_buffer = splitAndSort(buffer,sectors_array,sectors_array_len);
	free(buffer);
	return final_buffer;
}

char* PPDINTERFACE_readSectors2(uint32_t* sectors_toRead, size_t sectors_toRead_len)
{

	/* BUSQUEDA DE UN SOCKET LIBRE */
	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = PPDINTERFACE_getFreeSocket();
	pthread_mutex_lock(&ppd_socket->sock_mutex);
	/* FIN BUSQUEDA DE SOCKET LIBRE */

	size_t one_message_len = 11; //TIPO + LEN + PAYLOAD (ID PEDIDO, NUMERO SECTOR)
	size_t received_data_len = (boot_sector.bytes_perSector + one_message_len) * sectors_toRead_len;

	char* sectors_toRead_responses = malloc(received_data_len);
	char* sector_toRead_response;

	fd_set read_sockets_set;
	uint32_t received_responses = 0;

	uint32_t sectors_toRead_index = 0;
	uint32_t received_response_len = 0;

	for(sectors_toRead_index = 0;sectors_toRead_index < sectors_toRead_len;sectors_toRead_index++)
	{
		struct timeval val;
		val.tv_sec = 0;
		val.tv_usec = 0;

		FD_ZERO(&read_sockets_set);
		FD_SET(ppd_socket->descriptor,&read_sockets_set);

		if (select(ppd_socket->descriptor+1, &read_sockets_set,NULL,NULL,&val) > 0)
		{
			if(FD_ISSET(ppd_socket->descriptor,&read_sockets_set))
			{
				sector_toRead_response = COMM_receive(ppd_socket->descriptor,(uint32_t*) &received_response_len);

				if (received_response_len == -1)
				{
					perror("recv");
					exit(1);
				}
				else
				{
					memcpy(sectors_toRead_responses+(received_responses*received_response_len),sector_toRead_response,received_response_len);
					received_responses++;
					free(sector_toRead_response);
				}

			}
		}

		char* sector_toRead_request = createRequest(READ_SECTORS,sizeof(uint32_t),(char*) (sectors_toRead+sectors_toRead_index));

		if (COMM_send(sector_toRead_request,ppd_socket->descriptor) == -1)
		{
			perror("send");
			exit(1);
		}

		free(sector_toRead_request);
	}

	received_response_len=0;

	for(;received_responses<sectors_toRead_len;received_responses++)
	{
		sector_toRead_response = COMM_receive(ppd_socket->descriptor,&received_response_len);
		if (received_response_len == -1)
		{
			perror("recv");
			exit(1);
		}
		memcpy(sectors_toRead_responses+(received_responses*received_response_len),sector_toRead_response,received_response_len);
		free(sector_toRead_response);
	}

	char *sectors_readed_messages = splitAndSort(sectors_toRead_responses,sectors_toRead,sectors_toRead_len);
	free(sectors_toRead_responses);


	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);
	return sectors_readed_messages;
}

char* PPDINTERFACE_writeSectors(queue_t sectors_toWrite,size_t sectors_toWrite_len)
{
	/* BUSQUEDA DE UN SOCKET LIBRE */
	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = PPDINTERFACE_getFreeSocket();
	pthread_mutex_lock(&ppd_socket->sock_mutex);
	/* FIN BUSQUEDA DE SOCKET LIBRE */

	size_t sector_index = 0,responses_received = 0;
	size_t response_message_len = 523;
	queueNode_t *cur_sector_node = sectors_toWrite->begin;
	//char *buffer = malloc(response_message_len*sectors_array_len);

	for (;sector_index < sectors_toWrite_len;sector_index++)
	{
		char *msg_buf;
		int32_t readable_bytes = 0;

		if (ioctl(ppd_socket->descriptor,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{
				msg_buf = malloc(response_message_len);
				int32_t received = SOCKET_recvAll(ppd_socket->descriptor,msg_buf,response_message_len,MSG_DONTWAIT);

				if (received > 0)
				{
					responses_received++;
				}
				else if (received == SOCK_DISCONNECTED || received == SOCK_ERROR)
				{
					free(msg_buf);
					printf("ERROR: Se perdio la conexion con el otro extremo.\n");
					exit(-1);
				}
				free(msg_buf);
			}
		}

		sector_t *sector_toWrite = (sector_t*) cur_sector_node->data;

		msg_buf = malloc(523);

		*msg_buf = WRITE_SECTORS;
		*((uint16_t*) (msg_buf+1)) = 520;
		*((uint32_t*) (msg_buf+3)) = request_id;
		*((uint32_t*) (msg_buf+7)) = sector_toWrite->number;
		memcpy(msg_buf+11,sector_toWrite->data,boot_sector.bytes_perSector);


		int32_t sent = SOCKET_sendAll(ppd_socket->descriptor,msg_buf,response_message_len,NULL);
		if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
		{
			printf("ERROR: Se perdio la conexion con el otro extremo.\n");
			exit(-1);
		}
		free(msg_buf);

		cur_sector_node = cur_sector_node->next;
	}

	for (;responses_received<sectors_toWrite_len;responses_received++)
	{
		char *msg_buf = malloc(response_message_len);
		int32_t received = SOCKET_recvAll(ppd_socket->descriptor,msg_buf,response_message_len,NULL);

		if (received > 0)
		{

		}
		else if (received == SOCK_DISCONNECTED || received == SOCK_ERROR)
		{
			free(msg_buf);
			printf("ERROR: Se perdio la conexion con el otro extremo.\n");
			exit(-1);
		}
		free(msg_buf);
	}

	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);


	return NULL;
}

char* PPDINTERFACE_writeSectors2(queue_t sectors)
{
	uint32_t sock_index = 0;
	uint32_t sector_count = QUEUE_length(&sectors);

	size_t msg_len = boot_sector.bytes_perSector + 11; //TIPO + LEN + PAYLOAD (IDPEDIDO,SECTOR,DATOS)
	size_t recvdata_len = msg_len * sector_count;

	sem_wait(&sockets_toPPD.free_sockets);
	for (;sock_index < sockets_toPPD.size;sock_index++)
	{
		if (sockets_toPPD.sockets[sock_index].status == SOCK_FREE)
		{
			sockets_toPPD.sockets[sock_index].status = SOCK_NOTFREE;
			break;
		}
	}

	queueNode_t *sector_node;
	uint32_t count_sector = 0;

	pthread_mutex_lock(&sockets_toPPD.sockets[sock_index].sock_mutex);

	COMM_sendAdvise(sockets_toPPD.sockets[sock_index].descriptor,msg_len*sector_count,msg_len);

	while ((sector_node = QUEUE_takeNode(&sectors)) != NULL)
	{
		char* payload = malloc(4+boot_sector.bytes_perSector);
		sector_t* sector = (sector_t*) sector_node->data;
		memcpy(payload,&sector->number,4);
		memcpy(payload+4,sector->data,boot_sector.bytes_perSector);


		char* msg = createRequest(WRITE_SECTORS,sizeof(uint32_t)+boot_sector.bytes_perSector, payload);



		uint32_t res = sendMsgToPPD(sockets_toPPD.sockets[sock_index],msg);
		free(payload);
		free(msg);

		count_sector++;
	}


	char* msgs_buf = malloc(recvdata_len);

	uint32_t rec = 0;
	if ((rec = recv(sockets_toPPD.sockets[sock_index].descriptor,msgs_buf,recvdata_len,MSG_WAITALL)) < recvdata_len)
	{
		printf("ERROR AL RECIBIR ESCRITURA DE SECTORES");
		exit(0);
	}


	sockets_toPPD.sockets[sock_index].status = SOCK_FREE;
	pthread_mutex_unlock(&sockets_toPPD.sockets[sock_index].sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);
	return NULL;
}

socketPool_t create_connections_pool(uint32_t max_conn,char* address,uint32_t port)
{
	socketPool_t sock_pool;
	sock_pool.size = max_conn;
	sock_pool.sockets = malloc(sizeof(socketInet_t)*max_conn);
	sem_init(&sock_pool.free_sockets,NULL,max_conn);
	uint32_t index = 0;

	for (;index < max_conn;index++)
	{
		sock_pool.sockets[index] = SOCKET_inet_create(SOCK_STREAM,address,port,MODE_CONNECT);

		if (sock_pool.sockets[index].status == SOCK_OK)
		{
			char* handshake = malloc(3);
			memset(handshake,0,3);
			send(sock_pool.sockets[index].descriptor,handshake,3,NULL);
			handshake = realloc(handshake,3);
			recv(sock_pool.sockets[index].descriptor,handshake,3,NULL);
			if (*(handshake+1) != 0x00)
			{
				printf("ERROR POR HANDSHAKE");
				exit(0);
			}
			free(handshake);
			pthread_mutex_init(&sock_pool.sockets[index].sock_mutex,NULL);
			sock_pool.sockets[index].status = SOCK_FREE;
		}
		else
		{
			sock_pool.size = 0;
			return sock_pool;
		}
	}

	return sock_pool;

}

int32_t sendMsgToPPD(socketInet_t socket,char *msg)
{
	uint32_t len = *((uint16_t*) (msg+1)) + 3 ;

	uint32_t transmitted = send(socket.descriptor,msg,len,NULL);
	//free(msg_inBytes);
	if (transmitted != len)
	{
		return errno;
	}
	else
	{
		return transmitted;
	}
}

char* splitAndSort(char *sectors,uint32_t *indexes_array,size_t array_len)
{
	size_t msg_len = boot_sector.bytes_perSector + 11;
	uint32_t index = 0,index2 = 0;
	char* buf = malloc(boot_sector.bytes_perSector * array_len);
	for (;index < array_len;index++)
	{
		for (;index2 < array_len;index2++)
		{
			if (indexes_array[index] == *((uint32_t*) (sectors+(index2*msg_len)+7)))
			{
				memcpy(buf+(index*boot_sector.bytes_perSector),sectors+(index2*msg_len)+11,boot_sector.bytes_perSector);
			}
		}
	}
	return buf;
}

char* createRequest(NIPC_type msg_type,uint32_t payload_len,char* payload)
{
	char* msg = malloc(payload_len+(sizeof(char)*3)+sizeof(uint32_t)); //TIPO + LEN + PAYLOAD
	memcpy(msg,&msg_type,1);

	uint16_t len = payload_len+sizeof(uint32_t); //PAYLOAD + IDPEDIDO
	memcpy(msg+1,&len,2);

	memcpy(msg+3,&request_id,sizeof(uint32_t));
	memcpy(msg+7,payload,payload_len);
	request_id++;
	return msg;
}

socketInet_t* PPDINTERFACE_getFreeSocket()
{
	uint32_t sockets_toPPD_index = 0;

	for (;sockets_toPPD_index < sockets_toPPD.size;sockets_toPPD_index++)
	{
		if (sockets_toPPD.sockets[sockets_toPPD_index].status == SOCK_FREE)
		{
			sockets_toPPD.sockets[sockets_toPPD_index].status = SOCK_NOTFREE;
			return (sockets_toPPD.sockets+sockets_toPPD_index);
		}
	}
}

