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
#include <stdio.h>
#include "pfs_fat32.h"

typedef struct cluster_node
{
	uint32_t number;
	struct cluster_node* next;

} cluster_node;

//saque la definicion de FAT_struct xq ya esta definido en pfs_fat32, si queres sacalo de ahi y ponelo aca

// fat32_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
cluster_node* FAT_getClusterChain(FAT_struct *fat,uint32_t first_cluster);

//FAT32_getFreeClusters: Obtiene una lista de clusters libres
uint32_t FAT32_getFreeClusters(cluster_node* first);

#endif /* TAD_FAT_H_ */
