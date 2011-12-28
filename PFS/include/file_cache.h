/*
 * file_cache.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef FILE_CACHE_H_
#define FILE_CACHE_H_
#include <stdbool.h>
#include <pthread.h>
#include "tad_queue.h"
#include "tad_cluster.h"
#include "tad_file.h"



typedef struct cache_block
{
	uint32_t cluster_no;
	uint32_t size;
	char* data;
	time_t timestamp;
	uint32_t uses;
	bool written;

} cache_block_t;

cluster_t* CACHE_write_block(queue_t *cache,cluster_t cluster);
cache_block_t* CACHE_read_block(queue_t *cache,uint32_t cluster_no);
cache_block_t* CACHE_getLRU(queue_t *cache);

#endif /* FILE_CACHE_H_ */
