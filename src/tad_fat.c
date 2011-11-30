/*
 * tad_fat.c
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */
#include "tad_fat.h"
#include "tad_sector.h"
#include "tad_bootsector.h"
#include "pfs_comm.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include "log.h"
#include <assert.h>

extern t_log* log_file;
extern bootSector_t boot_sector;

queue_t FAT_getClusterChain(fatTable_t *fat,uint32_t init_cluster)
{

	uint32_t root_cluster = 0x0FFFFFF8;
	uint32_t  *cluster_number, cluster_no = init_cluster;
	queueNode_t *new_cluster_node;
	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	uint32_t *casted_table = (uint32_t*) fat->table;
	if (casted_table[cluster_no] == 0x00 )
	{
		return cluster_list;
	}
	else if (casted_table[cluster_no] == fat->EOC || casted_table[cluster_no] == root_cluster)
	{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			//log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
			QUEUE_appendNode(&cluster_list,cluster_number);
	}
	else if (casted_table[cluster_no] != fat->EOC)
	{
		while (casted_table[cluster_no] != fat->EOC)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
			QUEUE_appendNode(&cluster_list,cluster_number);

			cluster_no = casted_table[cluster_no];
		}

		cluster_number = malloc(sizeof(uint32_t));
		*cluster_number = cluster_no;
		log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
		QUEUE_appendNode(&cluster_list,cluster_number);
	}


	return cluster_list;
}

queue_t FAT_getFreeClusters(fatTable_t* fat) {
	uint32_t cluster_no, *cluster_number;

	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	uint32_t *casted_table = (uint32_t*) fat->table;
	for(cluster_no = 2;cluster_no < fat->size;cluster_no++)
	{
		if(casted_table[cluster_no] == 0)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			QUEUE_appendNode(&cluster_list,cluster_number);
		}
	}


	return cluster_list;
}

uint32_t FAT_getFreeCluster(fatTable_t* fat)
{
	uint32_t cluster_no;
	uint32_t *casted_table = (uint32_t*) fat->table;
	for(cluster_no = 2;cluster_no < fat->size;cluster_no++)
	{
		if(casted_table[cluster_no] == 0)
		{
			return cluster_no;
		}
	}

	return 0; //DISCO LLENO
}

uint32_t FAT_setUsed(fatTable_t* fat,uint32_t clusterToSet)
{
	uint32_t cluster_no;
	uint32_t *casted_table = (uint32_t*) fat->table;
	casted_table[clusterToSet] = fat->EOC;
	/*for(cluster_no = 2;cluster_no < fat->size;cluster_no++)
	{
		if(casted_table[cluster_no] == clusterToSet)
		{
			casted_table[cluster_no] = fat->EOC;
			return 0;
		}
	}*/
	return 1;
}

uint32_t FAT_read(fatTable_t *fat)
{
	//log_debug(log_file,"PFS","Leyendo FAT Table");
	uint32_t bytes_perFATentry = 4;


	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	queue_t sectors;
	QUEUE_initialize(&sectors);
	uint32_t first_sector = 32;
	uint32_t last_sector =  31+boot_sector.sectors_perFat32;
	uint32_t cur_sector;
	//uint32_t sectors[boot_sector.sectors_perFat32];


	fat->size = (boot_sector.bytes_perSector*boot_sector.sectors_perFat32) / bytes_perFATentry;
	//fat->table = malloc(fat->size * 4);
	uint32_t sector_array[boot_sector.sectors_perFat32];
	for (cur_sector = first_sector; cur_sector <= last_sector; cur_sector++)
	{
		sector_array[cur_sector-32] = cur_sector;
		 //TODO: ARMAR UN ARRAY DE SECTORES Y MANDARLO A LA FUNCION PPDINTERFACE_readSectorS

	}
	uint32_t count = 0;
	fat->table = PPDINTERFACE_readSectors(sector_array,boot_sector.sectors_perFat32);

	for (cur_sector = first_sector; cur_sector <= last_sector; cur_sector++)
	{
		sector_t *new_sector = malloc(sizeof(sector_t));
		new_sector->number = cur_sector;
		new_sector->modified = false;
		new_sector->size = boot_sector.bytes_perSector;
		new_sector->data = fat->table+((cur_sector-32)*boot_sector.bytes_perSector);
		QUEUE_appendNode(&sectors,new_sector);
		count++;

	}
	uint32_t len = QUEUE_length(&sectors);




	fat->sectors = sectors;

	fat->EOC = *(((uint32_t*) fat->table)+1);

	assert(*((char*) fat->table) == boot_sector.media_descriptor);
	log_debug(log_file,"PFS","FAT Table OK");

 return 0;

}

void FAT_write(fatTable_t *fat)
{
	queueNode_t *cur_sector_node = fat->sectors.begin;
	PPDINTERFACE_writeSectors(fat->sectors,QUEUE_length(&fat->sectors));
	/*while (cur_sector_node != NULL)
	{
		sector_t *cur_sector = (sector_t*) cur_sector_node->data;
		//if (cur_sector->modified)
		PPDINTERFACE_writeSector(*cur_sector);
		cur_sector_node = cur_sector_node->next;
	}*/
}

uint32_t FAT_appendCluster(fatTable_t fat,uint32_t first_cluster_of_chain)
{
	uint32_t *casted_table = (uint32_t*) fat.table;
	queue_t free_clusters = FAT_getFreeClusters(&fat);
	queue_t cluster_chain = FAT_getClusterChain(&fat,first_cluster_of_chain);
	queueNode_t *cur_free_cluster,*cur_cluster_node;
	uint32_t last_cluster;

	while ((cur_cluster_node = QUEUE_takeNode(&cluster_chain)) != NULL)
	{
		last_cluster = *((uint32_t*) cur_cluster_node->data);
		QUEUE_freeNode(cur_cluster_node);
	}



	if ((cur_free_cluster = QUEUE_takeNode(&free_clusters)) != NULL)
	{
		uint32_t appended_cl = casted_table[last_cluster] = *((uint32_t*) cur_free_cluster->data);
		casted_table[casted_table[last_cluster]] = fat.EOC;
		while ((cur_free_cluster = QUEUE_takeNode(&free_clusters)) != NULL)
		{
			QUEUE_freeNode(cur_free_cluster);
		}
		return appended_cl;
	}
	return 1;
}

uint32_t FAT_removeCluster(fatTable_t fat,uint32_t first_cluster_of_chain)
{
		uint32_t *casted_table = (uint32_t*) fat.table;
		queue_t cluster_chain = FAT_getClusterChain(&fat,first_cluster_of_chain);
		queueNode_t *cur_free_cluster,*cur_cluster_node;
		uint32_t last_cluster,before_lastcluster;

		while ((cur_cluster_node = QUEUE_takeNode(&cluster_chain)) != NULL)
		{
			last_cluster = *((uint32_t*) cur_cluster_node->data);
			if (cur_cluster_node->next == cluster_chain.end && cluster_chain.end != NULL) before_lastcluster = last_cluster;
			QUEUE_freeNode(cur_cluster_node);
		}

		casted_table[last_cluster] = 0;
		casted_table[before_lastcluster] = fat.EOC;
}

uint32_t FAT_getNextAssociated(fatTable_t fat,uint32_t cluster_no)
{
	if (cluster_no == fat.EOC) return fat.EOC;
	uint32_t *casted_table = (uint32_t*) fat.table;
	queue_t cluster_chain = FAT_getClusterChain(&fat,cluster_no);

	queueNode_t *cur_free_cluster,*cur_cluster_node;
	uint32_t cur_cluster,before_lastcluster;

	while ((cur_cluster_node = QUEUE_takeNode(&cluster_chain)) != NULL)
	{
		cur_cluster = *((uint32_t*) cur_cluster_node->data);
		if (cur_cluster == cluster_no) return *((uint32_t*) cur_cluster_node->next->data);
		QUEUE_freeNode(cur_cluster_node);
	}

}
