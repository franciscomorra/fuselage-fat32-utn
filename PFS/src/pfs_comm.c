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
#include "tad_bootsector.h"

//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

extern BS_struct boot_sector;

char* PFS_requestSectorsOperation(NIPC_type request_type,uint32_t *sectors,size_t sectors_count)
{
	NIPC_msg msg = NIPC_createMsg(request_type,sectors_count*sizeof(uint32_t),(char*) sectors);
	return PFS_request(msg);
}

char* PFS_request(NIPC_msg msg)
{

	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET
	//CAMBIAR POR ENVIO POR SOCKET

	if (msg.type == READ_SECTORS)
	{
		uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL

		size_t bytes_count = 0;
		memcpy(&bytes_count,msg.len,2);

		size_t sectors_count = bytes_count / 4;
		uint32_t index;
		char *buf = malloc(boot_sector.bytes_perSector*sectors_count);
		uint32_t *sectors_in_payload = (uint32_t*) msg.payload;
		for (index=0;index<sectors_count;index++)
		{
			//ERROR GRAVE: Payload muy grande
			read_sector(file_descriptor, sectors_in_payload[index],buf+(boot_sector.bytes_perSector*index));
		}
		close(file_descriptor);
		return buf;

	}
	else if (msg.type == WRITE_SECTORS)
	{

	}
}

