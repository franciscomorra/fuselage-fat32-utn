/*
 * tad_bootsector.h
 *
 *  Created on: 28/09/2011
 *      Author: utn_so
 */

#ifndef TAD_BOOTSECTOR_H_
#define TAD_BOOTSECTOR_H_

#include <stdint.h>

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
} __attribute__((__packed__)) BOOT_SECTOR;
//___STRUCT_BOOT_SECTOR_STRUCT

#endif /* TAD_BOOTSECTOR_H_ */
