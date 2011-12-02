/*
 * tad_fat.h
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FAT_H_
#define TAD_FAT_H_

#include "tad_queue.h"
#include <stdint.h>
#include <stdlib.h>
#include "tad_sector.h"

//___STRUCT_FAT_TABLE
typedef struct {
	char *table;
	queue_t sectors;
	size_t size;
	uint32_t EOC;
} fatTable_t;
//___STRUCT_FAT_TABLE

uint32_t FAT_read(fatTable_t *fat);

void FAT_write(fatTable_t *fat);
//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
queue_t FAT_getClusterChain(fatTable_t *fat,uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
queue_t FAT_getFreeClusters(fatTable_t *fat);

uint32_t FAT_getFreeCluster(fatTable_t* fat);

void FAT_setUsed(fatTable_t*fat,uint32_t clusterToSet);

void FAT_setFree(fatTable_t *fat,uint32_t clusterToSet);

uint32_t FAT_appendCluster(fatTable_t *fat,uint32_t first_cluster_of_chain);

uint32_t FAT_removeCluster(fatTable_t *fat,uint32_t first_cluster_of_chain);

uint32_t FAT_getNextAssociated(fatTable_t *fat,uint32_t cluster_no);

sector_t* FAT_searchSectorByPointer(queue_t fat_sector_list,char* pointer_to_search);
#endif /* TAD_FAT_H_ */
