/*
 * file_cache.c
 *
 *  Created on: 06/11/2011
 *      Author: utn_so
 */


#include "file_cache.h"
#include "tad_cluster.h"
#include "tad_queue.h"
#include <string.h>
#include <time.h>

uint32_t cache_size_inBytes;

cache_block_t* CACHE_getLRU(queue_t *cache)
{
	uint32_t last_uses = 9999999;
	queueNode_t *curr_block_node = cache->begin;
	cache_block_t* selected_block = NULL;
	while (curr_block_node != NULL)
	{
		cache_block_t* cur_block = (cache_block_t*) curr_block_node->data;
		if (cur_block->uses < last_uses)
		{
			selected_block = cur_block;
			last_uses = cur_block->uses;
		}
		curr_block_node = curr_block_node->next;
	}
	return selected_block;
}


cache_block_t* CACHE_read_block(queue_t *cache,uint32_t cluster_no)
{
	if (cache_size_inBytes == 0) return NULL;

	queueNode_t *curr_block_node = cache->begin;

	while (curr_block_node != NULL)
	{
		cache_block_t* cur_block = (cache_block_t*) curr_block_node->data;
		if (cur_block->cluster_no == cluster_no)
		{
			cur_block->uses++;
			return cur_block;
		}
		curr_block_node = curr_block_node->next;
	}
	return NULL;
}

cluster_t* CACHE_write_block(queue_t *cache,cluster_t cluster)
{
	if (cache_size_inBytes == 0) return NULL;
	queueNode_t *curr_block_node = cache->begin;
	bool block_exists = false;

	while (curr_block_node != NULL)
	{
		cache_block_t* cur_block = (cache_block_t*) curr_block_node->data;
		if (cur_block->cluster_no == cluster.number)
		{
			block_exists = true;
			memcpy(cur_block->data,cluster.data,4096);
			cur_block->uses++;
			return NULL;
		}
		curr_block_node = curr_block_node->next;
	}

	if (block_exists == false)
	{
		uint32_t max_blocks = cache_size_inBytes/4096;
		if (QUEUE_length(cache) == max_blocks)
		{
			cache_block_t *selected_block = CACHE_getLRU(cache);

			cluster_t* cluster_to_write = CLUSTER_newCluster(selected_block->data,selected_block->cluster_no);

			selected_block->cluster_no = cluster.number;
			selected_block->data = malloc(4096);
			memcpy(selected_block->data,cluster.data,4096);
			selected_block->uses = 0;
			selected_block->timestamp = time(NULL);

			return cluster_to_write;
		}
		else
		{
			cache_block_t* new_block = malloc(sizeof(cache_block_t));
			new_block->cluster_no = cluster.number;
			new_block->timestamp = time(NULL);
			new_block->uses = 0;
			new_block->written = false;
			new_block->data = malloc(4096);
			memcpy(new_block->data,cluster.data,4096);
			QUEUE_appendNode(cache,new_block);
			return NULL;
		}
	}
	return NULL;
}


