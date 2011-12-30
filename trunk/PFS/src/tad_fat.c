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

	uint32_t  *cluster_number, cluster_no = init_cluster;

	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	uint32_t *casted_table = (uint32_t*) fat.table;

	if (cluster_no == 0 || cluster_no == 1 || casted_table[cluster_no] == 0x00 )
	{
		pthread_mutex_unlock(&fat.mutex);
		return cluster_list;

	}
	else if (FAT_isEOC(casted_table[cluster_no]))
	{
		cluster_number = malloc(sizeof(uint32_t));
		*cluster_number = cluster_no;
		QUEUE_appendNode(&cluster_list,cluster_number);
	}
	else if (casted_table[cluster_no] != fat.EOC)
	{
		while (FAT_isEOC(casted_table[cluster_no]) == 0)
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

	uint32_t *casted_table = (uint32_t*) fat.table;
	uint32_t cluster_no = 2;

	for(cluster_no = 2;cluster_no < fat.size;cluster_no++)
	{
		if (casted_table[cluster_no] == 0)
		{
			pthread_mutex_unlock(&fat.mutex);
			return cluster_no;
		}
	}

	pthread_mutex_unlock(&fat.mutex);
	return 0;
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
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;

	uint32_t last_cluster = first_cluster_of_chain;
	uint32_t appended_cluster_number, free_cluster_number = 2;

	while (FAT_isEOC(casted_table[last_cluster]) == 0)
	{
		last_cluster = casted_table[last_cluster];
	}

	while (casted_table[free_cluster_number] != 0)
	{
		free_cluster_number++;
	}

	appended_cluster_number = casted_table[last_cluster] = free_cluster_number;
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
	pthread_mutex_lock(&fat.mutex);
	uint32_t *casted_table = (uint32_t*) fat.table;

	if (FAT_isEOC(casted_table[first_cluster_of_chain]))
	{
		casted_table[first_cluster_of_chain] = 0;
		sector_t *sector_modified = FAT_searchSectorByPointer((char*) (casted_table+first_cluster_of_chain));
		sector_modified->modified = true;
		pthread_mutex_unlock(&fat.mutex);
		return 0;
	}
	else
	{
		uint32_t removed_cluster, before_lastcluster, last_cluster = first_cluster_of_chain;

		while (FAT_isEOC(casted_table[last_cluster]) == 0)
		{
			before_lastcluster = last_cluster;
			last_cluster = casted_table[last_cluster];
		}

		casted_table[last_cluster] = 0;
		removed_cluster = last_cluster;
		casted_table[before_lastcluster] = fat.EOC;

		sector_t *first_sector_modified = FAT_searchSectorByPointer((char*) (casted_table+before_lastcluster));
		sector_t *second_sector_modified = FAT_searchSectorByPointer((char*) (casted_table+last_cluster));

		first_sector_modified->modified = true;
		second_sector_modified->modified = true;

		pthread_mutex_unlock(&fat.mutex);
		return removed_cluster;
	}

}

uint32_t FAT_get_next_linked(uint32_t cluster_no)
{
	pthread_mutex_lock(&fat.mutex);
	if (FAT_isEOC(cluster_no))
	{
		pthread_mutex_unlock(&fat.mutex);
		return fat.EOC;
	}
	uint32_t *casted_table = (uint32_t*) fat.table;
	pthread_mutex_unlock(&fat.mutex);
	return casted_table[cluster_no];
}

uint32_t FAT_get_last_linked(uint32_t cluster_no)
{
	pthread_mutex_lock(&fat.mutex);
	if (FAT_isEOC(cluster_no))
	{
		pthread_mutex_unlock(&fat.mutex);
		return fat.EOC;
	}
	uint32_t *casted_table = (uint32_t*) fat.table;

	uint32_t last_cluster = cluster_no;

	while (FAT_isEOC(casted_table[last_cluster]) == 0)
	{
		last_cluster = casted_table[last_cluster];
	}

	pthread_mutex_unlock(&fat.mutex);
	return last_cluster;
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

uint32_t FAT_isEOC(uint32_t number)
{
	return (number >= 0x0FFFFFF8 && number <= 0x0FFFFFFF) ? 1 : 0;
}
