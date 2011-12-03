/*
 * pfs_addressing.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */


#include <stdint.h>
#include "tad_bootsector.h"
extern bootSector_t boot_sector;

uint32_t* cluster_to_sectors(uint32_t cluster)
{
	uint32_t first_sector_ofData = boot_sector.reserved_sectors+(boot_sector.fats_no*boot_sector.sectors_perFat32);
	uint32_t first_sector_ofCluster = first_sector_ofData+(cluster-2)*boot_sector.sectors_perCluster;
	uint32_t *sectors= malloc(boot_sector.sectors_perCluster*sizeof(uint32_t));
	memset(sectors,0,boot_sector.sectors_perCluster*sizeof(uint32_t));
	int index;

	for (index=0;index < boot_sector.sectors_perCluster;index++)
	{
		sectors[index] = first_sector_ofCluster+index;
	}
	return sectors;
}
