/*
 * pfs_fat32.h
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 *
 * __attribute__((__packed__)) :  Atributo para que use la menos cantidad de bytes posibles para este struct,
 * 								  sino los datos se corren porque el compilador agrega un
 * 								  espaciado de bytes para alinear los datos en memoria
 */


#ifndef PFS_FAT32_H_
#define PFS_FAT32_H_

#include <stdint.h>
#include <stdbool.h>
#include "tad_fat.h"
#include "tad_file.h"
#include "tad_bootsector.h"
#include "tad_cluster.h"
#include "tad_queue.h"
#include "tad_direntry.h"



//cluster_set_t fat32_readClusterChain(uint32_t first_cluster);

void fat32_writeCluster(cluster_t *cluster);

queue_t fat32_readDirectory(const char* path);

fat32file_t* fat32_getFileEntry(const char* path);

cluster_t fat32_readCluster(uint32_t cluster_number);

uint32_t fat32_truncate(const char* fullpath,off_t new_size);

void fat32_remove(const char* path);

uint32_t fat32_mk(const char* fullpath,uint32_t dir_or_archive);

#endif /* PFS_FAT32_H_ */
