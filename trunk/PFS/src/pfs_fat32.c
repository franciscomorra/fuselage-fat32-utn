/*
 * pfs_fat32.c
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 */
#include "pfs_comm.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

uint32_t fat32_readFAT(FAT_struct *fat,uint32_t sectors_per_fat)
{
	fat->table = malloc(512*sectors_per_fat);
	fat->size = (512*sectors_per_fat);
	memset(fat->table,0,512*sectors_per_fat);

	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	uint32_t sectors[sectors_per_fat];
	int sector;
	for (sector = 32; sector <= 32+sectors_per_fat; sector++)
	{
		sectors[sector-32] = sector;
	}

	fat->table = (uint32_t*) PFS_requestSectorsRead(sectors,sectors_per_fat);
	return 0;
}


uint32_t fat32_readBootSector(BS_struct *bs)
{
	uint32_t sectors[1] = {0};
	char *bootsector_data = PFS_requestSectorsRead(sectors,1);
	memcpy(bs,bootsector_data,512);
	return 0;

}

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf)
{
	uint32_t *sectors = cluster_to_sectors(cluster_no);
	buf = PFS_requestSectorsRead(sectors,8);
	return 0;
}

