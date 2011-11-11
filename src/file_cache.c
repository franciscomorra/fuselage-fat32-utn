/*
 * file_cache.c
 *
 *  Created on: 06/11/2011
 *      Author: utn_so
 */


#include "file_cache.h"
#include "tad_cluster.h"
#include "tad_queue.h"
#include <time.h>

cluster_t* CACHE_writeFile(queue_t *file_caches,char* path,cluster_t cluster)
{
	queueNode_t	*curr_cache_node = file_caches->begin;
	bool cache_exists = false;
	while (curr_cache_node != NULL)
	{
		file_cache_t *cache = (file_cache_t*) curr_cache_node->data;
		if (strcmp(cache->path,path) == 0 )
		{
			cache_exists = true;
			queueNode_t *curr_block_node = cache->blocks.begin;
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
				uint32_t max_blocks = cache->cache_size/4096;
				if (QUEUE_length(&cache->blocks) == max_blocks)
				{
					cache_block_t *selected_block = CACHE_getLRU(&cache);

					//SACAR DE ACA?

					cluster_t* cluster_to_write = CLUSTER_newCluster(selected_block->data,selected_block->cluster_no);

					selected_block->cluster_no = cluster.number;
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

					QUEUE_appendNode(&cache->blocks,new_block);
					return NULL;
				}
			}
			break;
		}

			curr_cache_node = curr_cache_node->next;
	}

	if (cache_exists == false)
	{
		file_cache_t *new_cache = malloc(sizeof(file_cache_t));
		new_cache->path = malloc(strlen(path));
		strcpy(new_cache->path,path);
		new_cache->blocks.begin = new_cache->blocks.end	= NULL;
		new_cache->cache_size = 32768;
		QUEUE_appendNode(file_caches,new_cache);
		return CACHE_writeFile(file_caches,path,cluster);

	}
}


cache_block_t* CACHE_readFile(queue_t *file_caches,char* path,uint32_t cluster_no)
{
	queueNode_t *curr_cache_node = file_caches->begin;
	bool cache_exists = false;
	while (curr_cache_node != NULL)
	{
		file_cache_t *cache = (file_cache_t*) curr_cache_node->data;
		if (strcmp(cache->path,path) == 0)
		{
			queueNode_t *curr_block_node = cache->blocks.begin;
			bool block_exists = false;

			while (curr_block_node != NULL)
			{
				cache_block_t* cur_block = (cache_block_t*) curr_block_node->data;
				if (cur_block->cluster_no == cluster_no)
				{
					block_exists = true;
					cur_block->uses++;
					return cur_block;
				}
				curr_block_node = curr_block_node->next;
			}

			if (block_exists == false)
			{
				return NULL;
			}
		}
		curr_cache_node = curr_cache_node->next;
	}
	return NULL;
}

cache_block_t *CACHE_getLRU(file_cache_t *cache)
{
	uint32_t last_uses = 9999999;
	queueNode_t *curr_block_node = cache->blocks.begin;
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
