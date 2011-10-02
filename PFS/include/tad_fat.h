/*
 * tad_fat.h
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FAT_H_
#define TAD_FAT_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct CLUSTER_NODE
{
	uint32_t number;
	struct cluster_node * next;

} CLUSTER_NODE;

//___STRUCT_FAT_TABLE
typedef struct {
	uint32_t *table;
	size_t size;
	uint32_t EOC;
} FAT_TABLE;
//___STRUCT_FAT_TABLE



//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
CLUSTER_NODE* FAT_getClusterChain(FAT_TABLE *fat,uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
CLUSTER_NODE* FAT_getFreeClusters(FAT_TABLE* FAT);

//FAT_takeCluster: Saca un nodo de una lista de clusters
uint32_t FAT_takeCluster(CLUSTER_NODE* first, uint32_t clusterNumber);

//FAT_addCluster: Agrega un nodo a una lista de clusters
uint32_t FAT_addCluster(CLUSTER_NODE* first, CLUSTER_NODE* new);

void FAT_cleanList(CLUSTER_NODE* first);

#endif /* TAD_FAT_H_ */
