/*
 * tad_cluster.h
 *
 *  Created on: 20/10/2011
 *      Author: utn_so
 */

#ifndef TAD_CLUSTER_H_
#define TAD_CLUSTER_H_

#include "tad_queue.h"

typedef struct cluster_t
{
	uint32_t number;
	size_t size;
	queue_t sectors;
} cluster_t;

#endif /* TAD_CLUSTER_H_ */
