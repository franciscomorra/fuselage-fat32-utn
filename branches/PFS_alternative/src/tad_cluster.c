/*
 * tad_cluster.c
 *
 *  Created on: 24/10/2011
 *      Author: utn_so
 */

#include "tad_cluster.h"
#include "tad_queue.h"
#include "tad_sector.h"
#include <stdbool.h>
#include "pfs_addressing.h"
#include "tad_bootsector.h"

extern bootSector_t boot_sector;

void CLUSTER_freeQueue(queue_t *cluster_queue)
{
	queueNode_t *cur_cluster_node, *cur_sector_node;
	queue_t sector_queue;
	sector_t *cur_sector;
	cluster_t *cur_cluster;
	while ((cur_cluster_node = QUEUE_takeNode(cluster_queue)) != NULL)
	{
		cur_cluster = (cluster_t*) cur_cluster_node->data;
		CLUSTER_free(cur_cluster);
		QUEUE_freeNode(cur_cluster_node);
	}
}

void CLUSTER_free(cluster_t* cluster)
{
	queueNode_t *cur_sector_node;
	sector_t* cur_sector;
	while ((cur_sector_node = QUEUE_takeNode(&cluster->sectors)))
	{
		cur_sector = (sector_t*) cur_sector_node->data;
		//free(cur_sector->data);
		free(cur_sector);
		free(cur_sector_node);
	}
	free(cluster->data);
}

cluster_t* CLUSTER_newCluster(char* startOfData,uint32_t numberOfCluster)
{
	queue_t sector_queue;
	QUEUE_initialize(&sector_queue);
	uint32_t sector_index = 0;
	queueNode_t *new_sector_node;
	sector_t *new_sector;
	uint32_t *sectors = cluster_to_sectors(numberOfCluster);

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

void CLUSTER_freeChain(cluster_set_t *cluster_chain)
{
	queue_t cluster_queue = cluster_chain->clusters;
	queueNode_t *cluster_node;
	while ((cluster_node = QUEUE_takeNode(&cluster_queue)) != NULL)
	{
		cluster_t *cluster = (cluster_t*) cluster_node->data;
		queueNode_t* sector_node;
		while ((sector_node = QUEUE_takeNode(&(cluster->sectors))) != NULL)
		{
			free(sector_node->data);
			free(sector_node);
		}
		free(cluster_node->data);
		free(cluster_node);
	}
	free(cluster_chain->data);
}



uint32_t CLUSTER_setModified2(char *addr,cluster_set_t *cluster_chain,size_t len_modified) // + CANTIDAD MODIFICADA
{
		queue_t cluster_queue = cluster_chain->clusters;
		queueNode_t *cluster_node = (queueNode_t*) cluster_queue.begin;
		bool found = false;

	while (cluster_node != NULL)
		{
			cluster_t *cluster = (cluster_t*) cluster_node->data;
			queue_t sector_queue = cluster->sectors;
			queueNode_t *sector_node = (queueNode_t*)  sector_queue.begin;



			while (sector_node != NULL)
			{
				sector_t* sector = (sector_t*) sector_node->data;
				uint32_t addr_index;
				for (addr_index = 0; addr_index < boot_sector.bytes_perSector; addr_index++)
				{
					if ((sector->data+addr_index) == addr)
					{


						sector->modified = true;
						cluster->modified = true;
						return 0;
					}
				}
				sector_node = sector_node->next;
			}
			cluster_node = cluster_node->next;
		}
		return 1;
}

/*uint32_t CLUSTER_setModified(char *addr,cluster_t *cluster,size_t len_modified)
{
			uint32_t addr_index;
			bool sector_found = false;
			sector_t *last_sector;

			for (addr_index=0; addr_index < len_modified; addr_index++)
			{
				sector_found = false;
				queueNode_t *cur_sector_node = cluster->sectors.begin;
				while (cur_sector_node != NULL && sector_found == false)
				{
					sector_t *cur_sector = (sector_t*) cur_sector_node.data;

					uint32_t sector_byte_index;
					if (cur_sector->data <= addr+addr_index && addr+addr_index <= cur_sector->data+boot_sector.bytes_perSector)
					{
						cur_sector->modified = true;
						sector_found = true;
					}
					cur_sector_node = cur_sector_node->next;
				}
			}
}*/
