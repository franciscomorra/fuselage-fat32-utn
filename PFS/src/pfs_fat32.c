/*
 * pfs_fat32.c
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 */
#include "ppd_io.h"
#include "pfs_fat32.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

uint32_t FAT32_readFAT(FAT_struct *fat)
{

	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL
	if (file_descriptor == -1)
		perror(open);
	int i;
	fat->table = malloc(512*1024*4);
	fat->size = 512*1024;
	memset(fat->table,0,512*1024);
	char* tmp = (char*) malloc(512);

	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	for (i = 32; i < 1057; i++)
	{
		read_sector(file_descriptor,i,tmp);
		memcpy((fat->table)+(512*(i-32)),tmp,512);


	}
	//-------
	free(tmp);
	close(file_descriptor); //TEMPORAL
	return 0;
}

uint32_t FAT32_getClusterChain(FAT_struct *fat,uint32_t first_cluster,char* cluster_chain)
{
	return 0;
}

uint32_t FAT32_readBootSector(BS_struct *bs)
{
	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR); //TEMPORAL

	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	read_sector(file_descriptor,0,bs);
	close(file_descriptor); //TEMPORAL
	return 0;

}



