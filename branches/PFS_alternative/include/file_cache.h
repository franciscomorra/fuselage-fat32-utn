/*
 * file_cache.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef FILE_CACHE_H_
#define FILE_CACHE_H_
#include <stdbool.h>
#include "tad_queue.h"
#include "tad_cluster.h"
typedef struct file_cache_t
{
	char* path;
	queue_t blocks;
	size_t cache_size;
} file_cache_t;

typedef struct cache_block
{
	uint32_t cluster_no;
	uint32_t size;
	char* data;
	time_t timestamp;
	uint32_t uses;
	bool written;

} cache_block_t;

cluster_t* CACHE_writeFile(queue_t *file_caches,const char* path,cluster_t cluster);
cache_block_t* CACHE_readFile(queue_t *files_cache,const char* path,uint32_t cluster_no);
cache_block_t *CACHE_getLRU(file_cache_t *cache);
//TODO: Implementar algoritmos de reemplazo de datos en la cache
//TODO: Implementar funciones de agregar, buscar y quitar nodos

#endif /* FILE_CACHE_H_ */
