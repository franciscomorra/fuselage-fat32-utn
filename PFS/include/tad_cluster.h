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

cluster_t* CLUSTER_newCluster(char* startOfData,uint32_t numberOfCluster);

void CLUSTER_free(cluster_t* cluster);

uint32_t* CLUSTER_to_sectors(uint32_t cluster);

#endif /* TAD_CLUSTER_H_ */



