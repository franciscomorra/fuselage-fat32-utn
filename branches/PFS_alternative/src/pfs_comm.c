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
#include <sys/file.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <assert.h>

#include "pfs_comm.h"
#include "comm.h"
#include "tad_sector.h"
#include "tad_bootsector.h"
#include "tad_sockets.h"

bootSector_t boot_sector;
socketPool_t sockets_toPPD;
uint32_t request_id = 1;

uint32_t ppd_read_boot_sector()
{
	char *msg_buf = malloc(11);
	*msg_buf = READ_SECTORS;
	*((uint16_t*) (msg_buf+1)) = 8;
	*((uint32_t*) (msg_buf+3)) = request_id++;
	*((uint32_t*) (msg_buf+7)) = 0;

	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = ppd_get_free_socket();
	pthread_mutex_lock(&ppd_socket->sock_mutex);

	int32_t sent = send(ppd_socket->descriptor,msg_buf,11,MSG_WAITALL);
	if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
	{
		printf("ERROR LEYENDO EL BOOT SECTOR.\n");
		exit(-1);
	}
	free(msg_buf);

	msg_buf = malloc(3);

	int32_t received = recv(ppd_socket->descriptor,msg_buf,3,MSG_WAITALL);

	if (received > 0)
	{
		uint16_t payload_len = *((uint16_t*)(msg_buf+1));
		msg_buf = realloc(msg_buf,3+payload_len);
		received = recv(ppd_socket->descriptor,msg_buf+3,payload_len,MSG_WAITALL);
	}
	else
	{
		printf("ERROR LEYENDO EL BOOT SECTOR.\n");
		exit(-1);
	}

	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);

	assert(*msg_buf != 0x00);

	memcpy(&boot_sector,msg_buf+11,512);

	free(msg_buf);
	return 0;

}

char* ppd_read_sectors(uint32_t* sectors_array, size_t sectors_array_len)
{
	/* BUSQUEDA DE UN SOCKET LIBRE */
	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = ppd_get_free_socket();

	pthread_mutex_lock(&ppd_socket->sock_mutex);
	ppd_socket->status = SOCK_NOTFREE;

	/* FIN BUSQUEDA DE SOCKET LIBRE */

	size_t responses_received = 0,requests_sent = 0;
	size_t response_message_len = 523;
	char *buffer = malloc(response_message_len*sectors_array_len);

	char *msg_buf;

	while (responses_received < sectors_array_len)
	{
			if (requests_sent < sectors_array_len)
			{
				msg_buf = malloc(11);

				*msg_buf = READ_SECTORS;
				*((uint16_t*) (msg_buf+1)) = 8;
				*((uint32_t*) (msg_buf+3)) = request_id++;
				*((uint32_t*) (msg_buf+7)) = sectors_array[requests_sent];

				int32_t sent = send(ppd_socket->descriptor,msg_buf,11,MSG_WAITALL);
				if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
				{
					printf("ERROR: Se perdio la conexion con el otro extremo.\n");
					exit(-1);
				}

				free(msg_buf);
				requests_sent++;
			}



		int32_t readable_bytes = 0;

		if (ioctl(ppd_socket->descriptor,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{
				msg_buf = malloc(response_message_len);

				int32_t received = recv(ppd_socket->descriptor,msg_buf,response_message_len,MSG_WAITALL);

				if (received > 0)
				{
					memcpy(buffer+(responses_received*response_message_len),msg_buf,response_message_len);
					responses_received++;
				}
				else if (received == -1 && errno != EWOULDBLOCK)
				{
					free(msg_buf);
					free(buffer);
					printf("ERROR: Se perdio la conexion con el otro extremo.\n");
					exit(-1);
				}
				free(msg_buf);
			}
		}
	}



	//printf("%d\n",getMicroseconds()-time1);
		//fflush(stdout);
	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);

		char *final_buffer = ppd_reconstruct_data_from_responses(buffer,sectors_array,sectors_array_len);

	free(buffer);
	return final_buffer;
}

char* ppd_write_sectors(queue_t sectors_toWrite,size_t sectors_toWrite_len)
{
	/* BUSQUEDA DE UN SOCKET LIBRE */
	sem_wait(&sockets_toPPD.free_sockets);
	socketInet_t *ppd_socket = ppd_get_free_socket();
	pthread_mutex_lock(&ppd_socket->sock_mutex);
	/* FIN BUSQUEDA DE SOCKET LIBRE */

	size_t responses_received = 0,requests_sent = 0;
	size_t response_message_len = 523;
	queueNode_t *cur_sector_node = sectors_toWrite.begin;

	while (responses_received < sectors_toWrite_len)
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

		if (requests_sent < sectors_toWrite_len)
		{
			sector_t *sector_toWrite = (sector_t*) cur_sector_node->data;

			msg_buf = malloc(523);

			*msg_buf = WRITE_SECTORS;
			*((uint16_t*) (msg_buf+1)) = 520;
			*((uint32_t*) (msg_buf+3)) = request_id++;
			*((uint32_t*) (msg_buf+7)) = sector_toWrite->number;
			memcpy(msg_buf+11,sector_toWrite->data,boot_sector.bytes_perSector);

			int32_t sent = SOCKET_sendAll(ppd_socket->descriptor,msg_buf,response_message_len,0);
			if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
			{
				printf("ERROR: Se perdio la conexion con el otro extremo.\n");
				exit(-1);
			}
			free(msg_buf);

			cur_sector_node = cur_sector_node->next;

			requests_sent++;
		}
	}

	ppd_socket->status = SOCK_FREE;
	pthread_mutex_unlock(&ppd_socket->sock_mutex);
	sem_post(&sockets_toPPD.free_sockets);


	return NULL;
}

socketPool_t ppd_create_connection_pool(uint32_t max_conn,char* address,uint32_t port)
{
	socketPool_t sock_pool;
	sock_pool.size = max_conn;
	sock_pool.sockets = malloc(sizeof(socketInet_t)*max_conn);
	sem_init(&sock_pool.free_sockets,0,max_conn);
	uint32_t index = 0;

	for (;index < max_conn;index++)
	{
		sock_pool.sockets[index] = SOCKET_inet_create(SOCK_STREAM,address,port,MODE_CONNECT);

		if (sock_pool.sockets[index].status == SOCK_OK)
		{
			char* handshake = malloc(3);
			memset(handshake,0,3);
			send(sock_pool.sockets[index].descriptor,handshake,3,0);
			handshake = realloc(handshake,3);
			recv(sock_pool.sockets[index].descriptor,handshake,3,0);
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

char* ppd_reconstruct_data_from_responses(char *sectors,uint32_t *indexes_array,size_t array_len)
{
	size_t msg_len = boot_sector.bytes_perSector + 11;
	uint32_t index = 0,index2 = 0;
	char* buf = malloc(boot_sector.bytes_perSector * array_len);
	for (;index < array_len;index++)
	{
		index2=0;
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

socketInet_t* ppd_get_free_socket()
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
	return NULL;
}

