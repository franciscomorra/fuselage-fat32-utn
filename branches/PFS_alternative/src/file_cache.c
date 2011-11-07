/*
 * file_cache.c
 *
 *  Created on: 06/11/2011
 *      Author: utn_so
 */


#include "file_cache.h"
#include "tad_cluster.h"
#include "tad_queue.h"

cluster_t CACHE_write(file_cache_t *cache,cluster_t block)
{
	uint32_t max_blocks = cache->cache_size/4096;
	if (QUEUE_length(&cache->blocks) == max_blocks)
	{
		queueNode_t *cur_cache_block = cache->blocks.begin;
		queueNode_t *selected_cache_block;
		uint32_t last_uses = 0, index =0;

		for (;index < max_blocks;index++)
		{
			cache_block_t* cur_block = (cache_block_t*) cur_cache_block->data;
			if (cur_block->uses < last_uses)
			{

				last_uses = cur_block->uses;
			}

			cur_cache_block = cur_cache_block->next;
		}

	}
}
