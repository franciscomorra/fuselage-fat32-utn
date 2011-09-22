/*
 * tad_fat.h
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FAT_H_
#define TAD_FAT_H_

#include <stdint.h>

typedef struct cluster_node
{
	uint32_t number;
	struct cluster_node * next;

} cluster_node;

//___STRUCT_FAT_TABLE
typedef struct {
	uint32_t *table;
	uint32_t size;

} FAT_struct;
//___STRUCT_FAT_TABLE



// fat32_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
cluster_node* FAT_getClusterChain(FAT_struct *fat,uint32_t first_cluster);

//FAT32_getFreeClusters: Obtiene una lista de clusters libres
uint32_t FAT_getFreeClusters(cluster_node* first,FAT_struct* FAT);

#endif /* TAD_FAT_H_ */
