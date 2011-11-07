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
//TODO: Implementar algoritmos de reemplazo de datos en la cache
//TODO: Implementar funciones de agregar, buscar y quitar nodos

#endif /* FILE_CACHE_H_ */
