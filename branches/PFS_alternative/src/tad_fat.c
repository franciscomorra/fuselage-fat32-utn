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

fat_table_t fat;

queue_t FAT_get_linked_clusters(uint32_t init_cluster)
{
	pthread_mutex_lock(&fat.mutex);

	uint32_t root_cluster = 0x0FFFFFF8;
	uint32_t  *cluster_number, cluster_no = init_cluster;

	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	uint32_t *casted_table = (uint32_t*) fat.table;
	if (casted_table[cluster_no] == 0x00 )
	{
		pthread_mutex_unlock(&fat.mutex);
		return cluster_list;
	}
	else if (casted_table[cluster_no] == fat.EOC || casted_table[cluster_no] == root_cluster)
	{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			//log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
			QUEUE_appendNode(&cluster_list,cluster_number);
	}
	else if (casted_table[cluster_no] != fat.EOC)
	{
		while (casted_table[cluster_no] != fat.EOC)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			QUEUE_appendNode(&cluster_list,cluster_number);

			cluster_no = casted_table[cluster_no];
		}

		cluster_number = malloc(sizeof(uint32_t));
		*cluster_number = cluster_no;
		QUEUE_appendNode(&cluster_list,cluster_number);
	}

	pthread_mutex_unlock(&fat.mutex);
	return cluster_list;
}

queue_t FAT_get_n_linked_clusters(uint32_t init_cluster,uint32_t linked_clusters_no)
{

	uint32_t cluster = init_cluster;
	uint32_t cluster_count = 1;
	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	QUEUE_appendNode(&cluster_list,&cluster);
	cluster = FAT_get_next_linked(cluster);

	while (cluster != fat.EOC && cluster_count < linked_clusters_no)
	{
		cluster = FAT_get_next_linked(cluster);
		QUEUE_appendNode(&cluster_list,&cluster);
		cluster_count++;
	}

	return cluster_list;
}

queue_t FAT_get_free_clusters()
{
	pthread_mutex_lock(&fat.mutex);
	uint32_t cluster_no = 2, *cluster_number;

	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);

	uint32_t *casted_table = (uint32_t*) fat.table;
	for(cluster_no = 2;cluster_no < fat.size;cluster_no++)
	{
		if(casted_table[cluster_no] == 0)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			QUEUE_appendNode(&cluster_list,cluster_number);
		}
	}
	pthread_mutex_unlock(&fat.mutex);
	return cluster_list;
}

uint32_t FAT_get_next_free_cluster()
{
	pthread_mutex_lock(&fat.mutex);
	uint32_t free_cluster_number = 2;
	uint32_t *casted_table = (uint32_t*) fat.table;

	while (casted_table[free_cluster_number] != 0)
	{
		free_cluster_number++;

		if (fat.size == free_cluster_number+1)
		{
			pthread_mutex_unlock(&fat.mutex);
			return 0;
		}


	}
	pthread_mutex_unlock(&fat.mutex);
	return free_cluster_number;
	/*for(cluster_no = 2;cluster_no < fat.size;cluster_no++)
	{
		if(casted_table[cluster_no] == 0)
		{
			pthread_mutex_unlock(&fat_mutex);
			return cluster_no;
		}
	}
*/

	return 0; //DISCO LLENO
}

void FAT_set_used_cluster(uint32_t clusterToSet)
{
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;
	casted_table[clusterToSet] = fat.EOC;

	sector_t *modified_sector = FAT_searchSectorByPointer((char*) (casted_table+clusterToSet));

	modified_sector->modified = true;
	pthread_mutex_unlock(&fat.mutex);

}

void FAT_set_free_cluster(uint32_t clusterToSet)
{
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;
	casted_table[clusterToSet] = 0;

	sector_t *modified_sector = FAT_searchSectorByPointer((char*) (casted_table+clusterToSet));
	modified_sector->modified = true;
	pthread_mutex_unlock(&fat.mutex);


}

uint32_t FAT_read_table()
{
	//log_debug(log_file,"PFS","Leyendo FAT Table");
	uint32_t bytes_perFATentry = 4;

	queue_t sectors;
	QUEUE_initialize(&sectors);
	pthread_mutex_init(&fat.mutex,NULL);
	uint32_t first_sector = 32;
	uint32_t last_sector =  31+boot_sector.sectors_perFat32;
	uint32_t cur_sector;

	fat.size = (boot_sector.bytes_perSector*boot_sector.sectors_perFat32) / bytes_perFATentry;

	uint32_t sector_array[boot_sector.sectors_perFat32];
	for (cur_sector = first_sector; cur_sector <= last_sector; cur_sector++)
	{
		sector_array[cur_sector-32] = cur_sector;
	}
	uint32_t count = 0;
	fat.table = ppd_read_sectors(sector_array,boot_sector.sectors_perFat32);

	for (cur_sector = first_sector; cur_sector <= last_sector; cur_sector++)
	{
		sector_t *new_sector = malloc(sizeof(sector_t));
		new_sector->number = cur_sector;
		new_sector->modified = false;
		new_sector->size = boot_sector.bytes_perSector;
		new_sector->data = fat.table+((cur_sector-32)*boot_sector.bytes_perSector);
		QUEUE_appendNode(&sectors,new_sector);
		count++;
	}

	fat.sectors = sectors;
	fat.EOC = *(((uint32_t*) fat.table)+1);

	assert(*((char*) fat.table) == boot_sector.media_descriptor);
	log_debug(log_file,"PFS","FAT Table OK");

 return 0;

}

void FAT_write_table()
{
	pthread_mutex_lock(&fat.mutex);
	queue_t sectors_to_write;
	QUEUE_initialize(&sectors_to_write);

	queueNode_t *cur_sector_node = fat.sectors.begin;
	size_t sectors_to_write_count = 0;
	while (cur_sector_node != NULL)
	{
		sector_t *cur_sector = (sector_t*) cur_sector_node->data;
		if (cur_sector->modified == true)
		{
			QUEUE_appendNode(&sectors_to_write,cur_sector);
			cur_sector->modified = false;
			sectors_to_write_count++;

		}
		cur_sector_node = cur_sector_node->next;
	}

	ppd_write_sectors(sectors_to_write,sectors_to_write_count);

	cur_sector_node = sectors_to_write.begin;
	queueNode_t *aux;
	while (cur_sector_node != NULL)
	{
		aux = cur_sector_node->next;
			free(cur_sector_node);
		cur_sector_node = aux;
	}
	pthread_mutex_unlock(&fat.mutex);
}

uint32_t FAT_link_free_cluster(uint32_t first_cluster_of_chain)
{

	//queue_t modified_sectors;
	//QUEUE_initialize(&modified_sectors);
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;

	//queue_t cluster_chain = FAT_get_linked_clusters(first_cluster_of_chain);
	//queueNode_t *cur_cluster_node;
	uint32_t last_cluster = first_cluster_of_chain;

	while (casted_table[last_cluster] != fat.EOC)
	{
		last_cluster = casted_table[last_cluster];
	}

	/*while ((cur_cluster_node = QUEUE_takeNode(&cluster_chain)) != NULL)
	{
		last_cluster = *((uint32_t*) cur_cluster_node->data);
		free(cur_cluster_node->data);
		free(cur_cluster_node);
	}
*/
	uint32_t free_cluster_number = 2;

	while (casted_table[free_cluster_number] != 0)
	{
		free_cluster_number++;
	}
	uint32_t appended_cluster_number = casted_table[last_cluster] = free_cluster_number;
	casted_table[free_cluster_number] = fat.EOC;

	sector_t *sector_modified = FAT_searchSectorByPointer((char*) (casted_table+last_cluster));
	sector_modified->modified = true;
	sector_modified = FAT_searchSectorByPointer((char*) (casted_table+free_cluster_number));
	sector_modified->modified = true;
	pthread_mutex_unlock(&fat.mutex);
	return appended_cluster_number;
}

uint32_t FAT_remove_last_linked_cluster(uint32_t first_cluster_of_chain)
{

//	queue_t modified_sectors;
//	QUEUE_initialize(&modified_sectors);
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;
	//queue_t cluster_chain = FAT_get_linked_clusters(first_cluster_of_chain);
	//queueNode_t *cur_cluster_node;
	uint32_t last_cluster = first_cluster_of_chain,before_lastcluster;

	if (casted_table[last_cluster] == fat.EOC)
	{
		casted_table[last_cluster] = 0;

		sector_t *sector_modified = FAT_searchSectorByPointer((char*) (casted_table+last_cluster));
		sector_modified->modified = true;
		//QUEUE_appendNode(&modified_sectors,first_sector_modified);
		//QUEUE_appendNode(&modified_sectors,second_sector_modified);
		pthread_mutex_unlock(&fat.mutex);
		return last_cluster;
	}


	while (casted_table[last_cluster] != fat.EOC)
	{
		before_lastcluster = last_cluster;
		last_cluster = casted_table[last_cluster];
	}

	/*while ((cur_cluster_node = QUEUE_takeNode(&cluster_chain)) != NULL)
	{
		last_cluster = *((uint32_t*) cur_cluster_node->data);
		if (cur_cluster_node->next == cluster_chain.end && cluster_chain.end != NULL) before_lastcluster = last_cluster;
		QUEUE_freeNode(cur_cluster_node);
	}*/

	casted_table[last_cluster] = 0;
	uint32_t removed_cluster = last_cluster;
	casted_table[before_lastcluster] = fat.EOC;

	sector_t *first_sector_modified = FAT_searchSectorByPointer((char*) (casted_table+before_lastcluster));
	sector_t *second_sector_modified = FAT_searchSectorByPointer((char*) (casted_table+last_cluster));

	first_sector_modified->modified = true;
	second_sector_modified->modified = true;
	//QUEUE_appendNode(&modified_sectors,first_sector_modified);
	//QUEUE_appendNode(&modified_sectors,second_sector_modified);
	pthread_mutex_unlock(&fat.mutex);
	return removed_cluster;
}

uint32_t FAT_get_next_linked(uint32_t cluster_no)
{pthread_mutex_lock(&fat.mutex);

	if (cluster_no == fat.EOC)
		{
		pthread_mutex_unlock(&fat.mutex);
			return fat.EOC;
		}
	uint32_t *casted_table = (uint32_t*) fat.table;
	//queue_t cluster_chain = FAT_get_linked_clusters(cluster_no);
	pthread_mutex_unlock(&fat.mutex);
	return casted_table[cluster_no];
}

//UTIL PARA SOLO ESCRIBIR LOS SECTORES QUE SE MODIFICAN
sector_t* FAT_searchSectorByPointer(char* pointer_to_search)
{
	queueNode_t* cur_sector_node = fat.sectors.begin;

	uint32_t max_fatentry_in_sector = boot_sector.bytes_perSector / sizeof(uint32_t);

	while (cur_sector_node != NULL)
	{
		sector_t *cur_sector = (sector_t*) cur_sector_node->data;

		uint32_t fatentry_in_sector_index = 0;
		for (;fatentry_in_sector_index < max_fatentry_in_sector;fatentry_in_sector_index++)
		{
			if (cur_sector->data+(fatentry_in_sector_index*sizeof(uint32_t)) == pointer_to_search)
			{
				return cur_sector;
			}
		}

		cur_sector_node = cur_sector_node->next;
	}
	return NULL;
}
