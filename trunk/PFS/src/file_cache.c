/*
 * file_cache.c
 *
 *  Created on: 05/11/2011
 *      Author: utn_so
 */


#include "file_cache.h"



void FILECACHE_newCache(queue_t cache_queue,char* path, clusterChain_t file_clusters)
{
	file_cache_t *new_cache = malloc(sizeof(file_cache_t));
	new_cache->file_clusters = file_clusters;


}
