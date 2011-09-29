/*
 * pfs_fat32.c
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 */
#include "pfs_comm.h"
#include "pfs_fat32.h"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

extern BS_struct boot_sector;

uint32_t fat32_readFAT(FAT_struct *fat)
{
	uint32_t bytes_perFATentry = 4;
	fat->size = (boot_sector.bytes_perSector*boot_sector.sectors_perFat32) / bytes_perFATentry;

	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	uint32_t sectors[boot_sector.sectors_perFat32];
	int sector;
	for (sector = 32; sector <= 32+boot_sector.sectors_perFat32; sector++)
	{
		sectors[sector-32] = sector;
	}

	fat->table = (uint32_t*) PFS_requestSectorsOperation(READ_SECTORS,sectors,boot_sector.sectors_perFat32);
	return 0;
}


uint32_t fat32_readBootSector(BS_struct *bs)
{
	uint32_t sectors[1] = {0} ;
	char *bootsector_data = PFS_requestSectorsOperation(READ_SECTORS,sectors,1);
	memcpy(bs,bootsector_data,512);
	free(bootsector_data);
	return 0;

}

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf)
{
	uint32_t *sectors = cluster_to_sectors(cluster_no);
	*buf = PFS_requestSectorsOperation(READ_SECTORS,sectors,8);
	free(sectors);
	return 0;
}

