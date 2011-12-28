/*
 * tad_cluster.h
 *
 *  Created on: 20/10/2011
 *      Author: utn_so
 */

#ifndef TAD_CLUSTER_H_
#define TAD_CLUSTER_H_

#include "tad_queue.h"
#include <stdbool.h>

typedef struct cluster_t
{
	uint32_t number;
	size_t size;
	queue_t sectors;
	bool modified;
	char* data;
} cluster_t;

typedef struct cluster_set_t
{
	size_t size;
	queue_t clusters;
	char* data;
} cluster_set_t;

void CLUSTER_freeQueue(queue_t *cluster_queue);

cluster_t* CLUSTER_newCluster(char* startOfData,uint32_t numberOfCluster);

void CLUSTER_freeChain(cluster_set_t *cluster_chain);

uint32_t CLUSTER_setModified(char *addr,cluster_set_t *cluster_chain,size_t len_modified);

void CLUSTER_free(cluster_t* cluster);

uint32_t* CLUSTER_to_sectors(uint32_t cluster);

#endif /* TAD_CLUSTER_H_ */



