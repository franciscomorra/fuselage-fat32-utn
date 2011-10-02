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

typedef struct cluster_node
{
	uint32_t number;
	struct cluster_node * next;

} cluster_node;

//___STRUCT_FAT_TABLE
typedef struct {
	uint32_t *table;
	size_t size;
	uint32_t EOC;

} FAT_struct;
//___STRUCT_FAT_TABLE



//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
cluster_node* FAT_getClusterChain(FAT_struct *fat,uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
cluster_node* FAT_getFreeClusters(FAT_struct* FAT);

//FAT_takeCluster: Saca un nodo de una lista de clusters
uint32_t FAT_takeCluster(cluster_node* first, uint32_t clusterNumber);

//FAT_addCluster: Agrega un nodo a una lista de clusters
uint32_t FAT_addCluster(cluster_node* first, cluster_node* new);

void FAT_cleanList(cluster_node* first);

#endif /* TAD_FAT_H_ */
