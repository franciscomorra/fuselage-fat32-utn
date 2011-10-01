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

#define EOC 0x0FFFFFF8;



//fat32_readFAT: Lee la tabla fat en la estructura FAT_struct
uint32_t fat32_readFAT(FAT_struct *fat);

//fat32_readBootSector: Lee el boot sector en la estructura BS_struct
uint32_t fat32_readBootSector(BS_struct *bs);

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf);

FILE_NODE* fat32_getFileList(char* cluster_data);

FILE_NODE* fat32_readDirectory(const char* path);
#endif /* PFS_FAT32_H_ */
