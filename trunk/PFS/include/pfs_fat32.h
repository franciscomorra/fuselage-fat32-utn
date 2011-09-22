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

#define EOC 0x0FFFFFF8;



//___STRUCT_BOOT_SECTOR_STRUCT
typedef struct
{
	char jmp[3];
	char oem_name[8];
	uint32_t bytes_perSector 		: 16;
	uint32_t sectors_perCluster 	: 8;
	uint32_t reserved_sectors		: 16;
	uint32_t fats_no				: 8;
	uint32_t roots_no				: 16;
	uint32_t total_sectors			: 16;
	char media_descriptor;
	uint32_t sectors_perFat12_16	: 16;
	uint32_t sectors_perTrack		: 16;
	uint32_t disk_heads				: 16;
	uint32_t hidden_sectors			: 32;
	uint32_t total_sectors2			: 32;
	uint32_t  sectors_perFat32		: 32;
	char fat_flags[2];
	char version[2];
	uint32_t root_cluster			: 32;
	uint32_t fsinfo_sector			: 16;
	uint32_t bootcopy_sector		: 16;
	char reserved[12];
	char physical_drive_number;
	char reserved2;
	char ext_boot_sign;
	uint32_t serial_number			: 32;
	char vol_label[11];
	char fat_type[8];
	char os_bootcode[420];
	char boot_sign[2];
} __attribute__((__packed__)) BS_struct;
//___STRUCT_BOOT_SECTOR_STRUCT



// fat32_readFAT: Lee la tabla fat en la estructura FAT_struct
uint32_t FAT32_readFAT(FAT_struct *fat,uint32_t sectors_per_fat);

//FAT32_readBootSector: Lee el boot sector en la estructura BS_struct
uint32_t FAT32_readBootSector(BS_struct *bs);

uint32_t FAT32_getClusterData(uint32_t cluster_no,char** buf);



#endif /* PFS_FAT32_H_ */