/*
 * pfs_comm.c
 *
 *  Created on: 08/09/2011
 *      Author: utn_so
 */

//PROBLEMAS A SOLUCIONAR: LINKEO CON COMMONS Y PASAJE DE LENGTH DE NIPC

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include "pfs_comm.h"
#include "ppd_io.h"
#include "nipc.h"

//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

msgNIPC_t PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count)
{
	msgNIPC_t msg;
	msg.type = READ;
	uint32_t index;

	msg.len = sectors_count*sizeof(uint32_t);//VER COMO SE PASA DE CHAR[2] A SIZE_T

	msg.payload = malloc(sectors_count*sizeof(uint32_t));
	for (index=0;index<sectors_count;index++)
	{
		memcpy(msg.payload+index,sectors+index,sizeof(uint32_t));
	}

	return msg;

}

msgNIPC_t PFS_requestSectorsWrite(uint32_t *sectors)
{

}

char* PFS_request(msgNIPC_t msg)
{
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET

	if (msg.type == READ)
	{
		uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL

		//size_t count = len/sizeof(uint32_t);
		size_t count = msg.len;//VER COMO SE PASA DE CHAR[2] A SIZE_T

		uint32_t i;
		char *buf = malloc(512*count);
		for (i=0;i<count;i++)
		{
			read_sector(file_descriptor,msg.payload[i],buf+(512*i));
		}
		close(file_descriptor);
		return buf;

	}
	else if (type == WRITE)
	{

	}
}

