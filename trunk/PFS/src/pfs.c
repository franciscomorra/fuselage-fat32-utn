/*
 * proceso_filesystem.c
 *
 *  Created on: 04/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include "fuse_operations.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "log.h"

bootSector_t boot_sector;
fatTable_t fat;
t_log *log;
fileNode_t *current_dir;

int main(int argc, char *argv[])
{
	log = log_create("PFS","pfs.log",DEBUG,M_CONSOLE_DISABLE);
	log_debug(log,"PFS","Inicio PFS");

	boot_sector.bytes_perSector = 512; //Habra que hacer alguna funcion especial para leer solo el boot_sector;
	fat32_readBootSector(&boot_sector);
	fat32_readFAT(&fat);

return fuse_main(argc, argv, &fuselage_oper,NULL);

}


int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	fileNode_t *file_list = fat32_readDirectory(path); //Obtengo una lista de los ficheros que hay en "path"
	log_debug(log,"PFS","Leyendo directorio %s",path);
	assert(file_list != NULL); //Si falla es que fuselage_getattr no detecto que no era un directorio valido
	fileNode_t *cur = file_list;
	while (cur != NULL)
	{
		log_debug(log,"PFS","Listando %s",cur->long_file_name);
		filler(buf, cur->long_file_name, NULL, 0);
		cur = cur->next;
	}
	FILENODE_cleanList(file_list);
	return 0;
}

static int fuselage_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	log_debug(log,"PFS","Obteniendo atributos de %s",path);
	if (strcmp(path,"/") == 0) //Si se solicitan los atributos del directorio raiz
	{
	  stbuf->st_mode = S_IFDIR | 0755;
	  stbuf->st_nlink = 2;
	  return res;
	}

	dirEntry_t *file =  fat32_getAttr(path); //Obtengo la dirEntry_t del fichero apuntado por path

	if (file == NULL) //SI DEVOLVIO NULL, EL ARCHIVO O DIRECTORIO NO EXISTE
	{
		log_debug(log,"PFS","No existe el fichero o directorio %s",path);
		res = -ENOENT;
	}
	else if (file->file_attribute.subdirectory == true) //Si es un directorio
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		free(file); //Libero la memoria de file
	}
	else if (file->file_attribute.archive == true) //Si es un archivo
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = file->file_size;
		free(file); //Libero la memoria de file
	}

	return res;
}

static int fuselage_open(const char *path, struct fuse_file_info *fi)
{
	printf(path);
	fflush(stdout);

}
