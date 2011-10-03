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
#include "tad_bootsector.h"
#include "tad_filenode.h"


//fat32_readFAT: Lee la tabla fat en la estructura FAT_struct
uint32_t fat32_readFAT(fatTable_t *fat);

//fat32_readBootSector: Lee el boot sector en la estructura BS_struct
uint32_t fat32_readBootSector(bootSector_t *bs);

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf);

fileNode_t* fat32_getFileList(char* cluster_data);

fileNode_t* fat32_readDirectory(const char* path);
#endif /* PFS_FAT32_H_ */
