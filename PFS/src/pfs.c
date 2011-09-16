/*
 * proceso_filesystem.c
 *
 *  Created on: 04/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdint.h>
#include "fuse_operations.h"
#include "pfs_comm.h"
#include "pfs_fat32.h"



int main(int argc, char *argv[])
{
	//SANTIAGO
	FAT_struct fat;
	FAT32_readFAT(&fat);

	BS_struct boot_sector;
	FAT32_readBootSector(&boot_sector);


	printf("%d",boot_sector.sectors_perFat32);

	//

	return 0;
 //return fuse_main(argc, argv, &fuselage_oper,NULL);
}


int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
		(void) offset;
	    (void) fi;


	    filler(buf, ".", NULL, 0);
	    filler(buf, "..", NULL, 0);
	    filler(buf, "HOLA MUNDO", NULL, 0);

	    return 0;
}


