/*
 * tad_cluster.c
 *
 *  Created on: 24/10/2011
 *      Author: utn_so
 */

#include <stdbool.h>
#include "tad_cluster.h"
#include "tad_queue.h"
#include "tad_sector.h"
#include "tad_bootsector.h"

extern bootSector_t boot_sector;

void CLUSTER_free(cluster_t* cluster)
{
	queueNode_t *cur_sector_node;
	while ((cur_sector_node = QUEUE_takeNode(&cluster->sectors)))
	{
		free(cur_sector_node->data);
		free(cur_sector_node);
	}
	free(cluster->data);
}

cluster_t* CLUSTER_newCluster(char* startOfData,uint32_t numberOfCluster)
{
	queue_t sector_queue;
	QUEUE_initialize(&sector_queue);
	uint32_t sector_index = 0;

	sector_t *new_sector;
	uint32_t *sectors = CLUSTER_to_sectors(numberOfCluster);

	for (sector_index = 0; sector_index < boot_sector.sectors_perCluster; sector_index++)
	{
		new_sector = malloc(sizeof(sector_t));
		new_sector->number = *(sectors + sector_index);
		new_sector->data = startOfData+(sector_index*boot_sector.bytes_perSector);
		new_sector->size = boot_sector.bytes_perSector;
		new_sector->modified = false;
		QUEUE_appendNode(&sector_queue,new_sector);

	}

	free(sectors);

	cluster_t *new_cluster = malloc(sizeof(cluster_t));
	new_cluster->data = startOfData;
	new_cluster->size = boot_sector.sectors_perCluster;
	new_cluster->number = numberOfCluster;
	new_cluster->sectors = sector_queue;
	new_cluster->modified = false;
	return new_cluster;

}

uint32_t* CLUSTER_to_sectors(uint32_t cluster)
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


