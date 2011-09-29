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
#include "pfs_comm.h"
#include "ppd_io.h"
#include "tad_bootsector.h"

//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

extern BS_struct boot_sector;

char* PFS_requestSectorsOperation(NIPC_type request_type,uint32_t *sectors,size_t sectors_count)
{
	uint32_t index;
	NIPC_msg msg;
	char *buf = malloc(sectors_count*boot_sector.bytes_perSector);
	char *tmp;

	for (index = 0;index < sectors_count;index++)
	{
		msg = NIPC_createMsg(request_type,sizeof(uint32_t), sectors+index);
		tmp = PFS_request(msg);
		NIPC_cleanMsg(&msg);
		memcpy(buf+(index*boot_sector.bytes_perSector),tmp,boot_sector.bytes_perSector);
		free(tmp);
	}

	return buf;
}

char* PFS_request(NIPC_msg msg)
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
	else if (msg.type == WRITE_SECTORS){

	}
	return 0;
}

