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
#include "pfs_comm.h"
#include "ppd_io.h"
#include "tad_sector.h"
#include <stdbool.h>
#include <semaphore.h>
#include "tad_bootsector.h"
#include "tad_sockets.h"
#include <sys/socket.h>
#include <errno.h>

//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

extern bootSector_t boot_sector;
extern socketPool_t sockets_toPPD;

char* PPDINTERFACE_readSector(uint32_t sector) //TODO CAMBIAR ESTA FUNCION POR UNA QUE RECIBA TODOS LOS SECTORES Y LOS PIDA AL PPD
{
	nipcMsg_t msg2 = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sector);
	char *buf1 = PFS_request(msg2);
	NIPC_cleanMsg(&msg2);
	return buf1;
}

char* PPDINTERFACE_readSectors(uint32_t* sectors, size_t len)
{
	uint32_t sock_index = 0;
	uint32_t sector_index = 0;
	size_t recvdata_len = (boot_sector.bytes_perSector + 7) * len;

	sem_wait(&sockets_toPPD.free_sockets);
	for (;sock_index < sockets_toPPD.size;sock_index++)
	{
		if (sockets_toPPD.sockets[sock_index].status == SOCK_FREE)
		{
			sockets_toPPD.sockets[sock_index].status = SOCK_NOTFREE;
			break;
		}
	}

	for (;sector_index < len;sector_index++)
	{
		nipcMsg_t msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) sectors+sector_index);
		sendMsgToPPD(msg);
		NIPC_cleanMsg(&msg);
	}

	char* buf = malloc(recvdata_len);
	if (recv(sockets_toPPD.sockets[sock_index].descriptor,buf,recvdata_len,NULL) == -1)
	{
		free(buf);
		buf = NULL;
	}
	sockets_toPPD.sockets[sock_index].status = SOCK_FREE;
	sem_post(&sockets_toPPD.free_sockets);
	return buf;
}

uint32_t PPDINTERFACE_writeSector(sector_t sector)
{
		nipcMsg_t msg;
		msg.type = WRITE_SECTORS;
		uint32_t len = 4 + boot_sector.bytes_perSector;
		memcpy(msg.len,&len,2);

		msg.payload = malloc(len);
		memcpy(msg.payload,&sector.number,4);
		memcpy(msg.payload+4,sector.data,boot_sector.bytes_perSector);
		PFS_request(msg);
		NIPC_cleanMsg(&msg);
		return 0;
}

char* PFS_requestSectorsOperation(NIPC_type request_type,uint32_t *sectors,size_t sectors_count)
{
	uint32_t index;
	nipcMsg_t msg;
	char *buf = malloc(sectors_count*boot_sector.bytes_perSector);
	memset(buf,0,sectors_count*boot_sector.bytes_perSector);
	char *tmp;

	for (index = 0;index < sectors_count;index++)
	{
		msg = NIPC_createMsg(request_type,sizeof(uint32_t),(char*)  (sectors+index));
		tmp = PFS_request(msg);
		NIPC_cleanMsg(&msg);
		memcpy(buf+(index*boot_sector.bytes_perSector),tmp,boot_sector.bytes_perSector);
		free(tmp);
	}

	return buf;
}

char* PFS_request(nipcMsg_t msg)
{
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET


	if (msg.type == READ_SECTORS)
	{
		uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL

		char *buf = malloc(boot_sector.bytes_perSector);
		uint32_t *sector = malloc(sizeof(uint32_t));
		memcpy(sector,msg.payload,4);
		read_sector(file_descriptor, *sector, buf);
		free(sector);

		close(file_descriptor);
		return buf;

	}
	else if (msg.type == WRITE_SECTORS)
	{
		uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL

				char *buf = malloc(boot_sector.bytes_perSector);
				uint32_t *sector = malloc(sizeof(uint32_t));
				memcpy(sector,msg.payload,4);
				memcpy(buf,msg.payload+4,boot_sector.bytes_perSector);
				write_sector(file_descriptor, *sector, buf);
				free(sector);
				free(buf);
				close(file_descriptor);
				return buf;
	}


	return 0;
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

int32_t sendMsgToPPD(socketInet_t socket,nipcMsg_t msg)
{
	char* msg_inBytes = NIPC_toBytes(msg);
	int32_t transmitted = 0;
	if ((transmitted = send(socket.descriptor,msg_inBytes,7,NULL)) == -1)
	{
		return errno;
	}
	else
	{
		return transmitted;
	}
}

