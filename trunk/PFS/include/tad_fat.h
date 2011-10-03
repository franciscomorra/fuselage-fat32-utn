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

} clusterNode_t;

//___STRUCT_FAT_TABLE
typedef struct {
	uint32_t *table;
	size_t size;
	uint32_t EOC;
} fatTable_t;
//___STRUCT_FAT_TABLE



//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
clusterNode_t* FAT_getClusterChain(fatTable_t *fat,uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
clusterNode_t* FAT_getFreeClusters(fatTable_t* FAT);

//FAT_takeCluster: Saca un nodo de una lista de clusters
uint32_t FAT_takeCluster(clusterNode_t* first, uint32_t clusterNumber);

//FAT_addCluster: Agrega un nodo a una lista de clusters
uint32_t FAT_addCluster(clusterNode_t* first, clusterNode_t* new);

void FAT_cleanList(clusterNode_t* first);

#endif /* TAD_FAT_H_ */
