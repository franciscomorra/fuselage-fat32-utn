/*
 * pfs_comm.c
 *
 *  Created on: 08/09/2011
 *      Author: utn_so
 */

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include "pfs_comm.h"
#include "ppd_io.h"
//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

char* PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count)
{
	char type = READ;
	uint32_t index;
	size_t len = sectors_count*sizeof(uint32_t);
	uint32_t* payload = malloc(sectors_count*sizeof(uint32_t));

	for (index=0;index<sectors_count;index++)
	{
		memcpy(payload+index,sectors+index,sizeof(uint32_t));

	}

	char* msg = malloc(1+2+len);
	memcpy(msg,&type,1);
	memcpy(msg+1,&len,2);
	memcpy(msg+3,payload,len);
	return PFS_request(msg);
}

uint32_t PFS_requestSectorsWrite(uint32_t *sectors)
{


}

char* PFS_request(char* msg)
{
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET

	//Descomponemos el mensaje en sus partes
	uint32_t len;
	char type = *msg;
	memcpy(&len,msg+1,2);
	uint32_t* payload = malloc(len);
	memcpy(payload,msg+3,len);
	//-----

	if (type == READ)
	{
		uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL
		size_t count = len/sizeof(uint32_t);
		uint32_t i;
		char *buf = malloc(512*count);
		for (i=0;i<count;i++)
		{
			read_sector(file_descriptor,payload[i],buf+(512*i));
		}
		close(file_descriptor);
		return buf;

	}
	else if (type == WRITE)
	{

	}
}

