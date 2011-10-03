/*
 * proceso_filesystem.c
 *
 *  Created on: 04/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdint.h>
#include "fuse_operations.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "log.h"

bootSector_t boot_sector;
fatTable_t fat;
t_log log;

int main(int argc, char *argv[])
{
	/*log = log_create("PFS","pfs_log.txt",);*/
	boot_sector.bytes_perSector = 512; //Habra que hacer alguna funcion especial para leer solo el boot_sector;
	fileNode_t *list = fat32_readDirectory("/");

	fat32_readBootSector(&boot_sector);
	fat32_readFAT(&fat);

	/*FAT_getClusterChain(&fat,9);

	clusterNode_t *first;
	first = FAT_getFreeClusters(&fat);
	FAT_cleanList(first);

	char *buf;
	fat32_getClusterData(2,&buf);

	FILENODE_cleanList(list);
	free(buf);

	//

return 0;*/
return fuse_main(argc, argv, &fuselage_oper,NULL);

}


int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
		(void) offset;
	    (void) fi;

	    fileNode_t *list = fat32_readDirectory(path);


	    if (list != 0x0)
	    {	fileNode_t *cur = list;
			while (cur != 0x0)
			{
				filler(buf, cur->long_file_name, NULL, 0);
				cur = cur->next;
			}
	    }
	    else
	    {

	    }


	    return 0;
}


