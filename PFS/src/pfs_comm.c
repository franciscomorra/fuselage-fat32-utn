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
#include "tad_bootsector.h"

//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

extern bootSector_t boot_sector;

char* PFS_sectorRead(uint32_t sector)
{
		nipcMsg_t msg;
		msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sector);
		char* buf = PFS_request(msg);
		NIPC_cleanMsg(&msg);
		return buf;
}

uint32_t PFS_sectorWrite(sector_t sector)
{
		nipcMsg_t msg;
		msg.type = WRITE_SECTORS;
		uint32_t len = 4 + boot_sector.bytes_perSector;
		memcpy(msg.len,&len,2);

		msg.payload = malloc(len);
		memcpy(msg.payload,sector.number,4);
		memcpy(msg.payload+4,sector.data,boot_sector.bytes_perSector);
		PFS_request(msg);
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
				close(file_descriptor);
				return buf;
	}
	return 0;
}

