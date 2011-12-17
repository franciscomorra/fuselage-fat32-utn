/*
 * tad_fat.h
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FAT_H_
#define TAD_FAT_H_

#include "tad_queue.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include "tad_sector.h"

//___STRUCT_FAT_TABLE
typedef struct {
	char *table;
	queue_t sectors;
	size_t size;
	uint32_t EOC;
	pthread_mutex_t mutex;
} fat_table_t;
//___STRUCT_FAT_TABLE

uint32_t FAT_read_table();

void FAT_write_table();
//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
queue_t FAT_get_linked_clusters(uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
queue_t FAT_get_free_clusters();

uint32_t FAT_get_next_free_cluster();

void FAT_set_used_cluster(uint32_t clusterToSet);

void FAT_set_free_cluster(uint32_t clusterToSet);

uint32_t FAT_link_free_cluster(uint32_t first_cluster_of_chain);

uint32_t FAT_remove_last_linked_cluster(uint32_t first_cluster_of_chain);

uint32_t FAT_get_next_linked(uint32_t cluster_no);

sector_t* FAT_searchSectorByPointer(char* pointer_to_search);
#endif /* TAD_FAT_H_ */
