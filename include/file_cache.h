/*
 * file_cache.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef FILE_CACHE_H_
#define FILE_CACHE_H_

typedef struct file_cache_node
{
	char* path;
	dirEntry_t direntry;
	struct file_cache_node *next;
} file_cache_node_t;

//TODO: Implementar algoritmos de reemplazo de datos en la cache
//TODO: Implementar funciones de agregar, buscar y quitar nodos

#endif /* FILE_CACHE_H_ */
