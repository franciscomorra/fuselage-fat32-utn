/*
 * tad_dirtable.h
 *
 *  Created on: 20/10/2011
 *      Author: utn_so
 */

#ifndef TAD_DIRTABLE_H_
#define TAD_DIRTABLE_H_

#include "tad_queue.h"
#include "stdbool.h"

typedef struct dirTable_t
{
	uint32_t first_cluster;
	uint32_t number_of_clusters;
	queue_t file_list;
	queue_t cluster_list;

} dirTable_t;




dirTable_t DIRTABLE_getTableForCluster(uint32_t cluster_number);

#endif /* TAD_DIRTABLE_H_ */
