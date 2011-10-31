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


//fat32_readFAT: Lee la tabla fat en la estructura FAT_struct
uint32_t fat32_readFAT(fatTable_t *fat);

//fat32_readBootSector: Lee el boot sector en la estructura BS_struct
uint32_t fat32_readBootSector(bootSector_t *bs);

char* fat32_getClusterRawData(uint32_t);

clusterChain_t fat32_getClusterChainData(uint32_t first_cluster);

queue_t fat32_readDirectory(const char* path,clusterChain_t *cluster_chain);

dirEntry_t* fat32_getDirEntry(char* path,clusterChain_t* cluster_chain);

void fat32_writeClusterData(cluster_t *cluster);

#endif /* PFS_FAT32_H_ */
