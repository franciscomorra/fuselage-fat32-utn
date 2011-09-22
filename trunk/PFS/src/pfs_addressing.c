/*
 * pfs_addressing.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */


#include <stdint.h>

uint32_t* cluster_to_sectors(uint32_t cluster)
{
	uint32_t first_sector_ofData = 32+(1024*2);
	uint32_t first_sector_ofCluster = first_sector_ofData+(cluster-2)*8;
	uint32_t *sectors = malloc(32);
	int index;

	for (index=0;index < 8;index++)
	{
		sectors[index] = first_sector_ofCluster+index;
	}
	return sectors;
}
