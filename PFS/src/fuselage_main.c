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

#include "fuselage_main.h"
#include "fuse_operations.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include "tad_list.h"
#include "tad_file.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "log.h"
#include <signal.h>
bootSector_t boot_sector;
fatTable_t fat;
t_log *log_file;
dirEntry_t *current_file;

extern 	struct args
{
	int argc;
	char **argv;
} args;

extern char* cmd_received;
int cmd_signal();
extern pthread_mutex_t signal_lock;


void *fuselage_main (void *data)
{

	log_file = log_create("PFS","pfs.log",INFO,M_CONSOLE_DISABLE);
	log_debug(log_file,"PFS","Inicio PFS");

	boot_sector.bytes_perSector = 512; //Habra que hacer alguna funcion especial para leer solo el boot_sector;


	fat32_readBootSector(&boot_sector);
	fat32_readFAT(&fat);
	listNode_t * list = FAT_getFreeClusters(&fat);
	signal(SIGUSR2,cmd_signal);
	return fuse_main(args.argc,args.argv, &fuselage_oper,NULL);
}

int cmd_signal()
{
	pthread_mutex_lock(&signal_lock);

	if (strcmp(cmd_received,"fsinfo") == 0)
	{
			listNode_t* cluster_list = FAT_getFreeClusters(&fat);
			listNode_t* cur = cluster_list;
			uint32_t count_free = 0;
			while (cur != NULL)
			{
				++count_free;
				cur = cur->next;
			}
			uint32_t count_notfree = fat.size - count_free;
			LIST_destroyList(&cluster_list,UINT32_T);

			printf("\nClusters ocupados: %d\n",count_notfree);
			printf("Clusters libres: %d\n",count_free);
			printf("Tamaño sector: %d bytes\n",boot_sector.bytes_perSector);
			printf("Tamaño cluster: %d bytes\n",(boot_sector.bytes_perSector*boot_sector.sectors_perCluster));
			printf("Tamaño FAT: %d Kb\n\n",(sizeof(uint32_t)*fat.size/1024));
			fflush(stdout);
	}
	//TODO: finfo [path a un archivo] obtener los 20 primeros clusters
	pthread_mutex_unlock(&signal_lock);
}

int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	log_debug(log_file,"PFS","fuselage_readdir -> fat32_readDirectory(%s)",path);
	listNode_t *file_list = fat32_readDirectory(path); //Obtengo una lista de los ficheros que hay en "path"

	assert(file_list != NULL); //Si falla es que fuselage_getattr no detecto que no era un directorio valido
	listNode_t *curr_file_node = file_list;
	fat32file_t *curr_file;
	while (curr_file_node != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;
		filler(buf, curr_file->long_file_name, NULL, 0);
		curr_file_node = curr_file_node->next;
	}

	log_debug(log_file,"PFS","fuselage_readdir -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
	LIST_destroyList(&file_list,FAT32FILE_T);

	return 0;
}

static int fuselage_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path,"/") == 0) //Si se solicitan los atributos del directorio raiz
	{
	  stbuf->st_mode = S_IFDIR | 0755;
	  stbuf->st_nlink = 2;
	  return res;
	}
	log_debug(log_file,"PFS","fuselage_getattr -> fat32_getDirEntry(%s)",path);
	current_file =  fat32_getDirEntry(path); //Obtengo la dirEntry_t del fichero apuntado por path

	if (current_file == NULL) //SI DEVOLVIO NULL, EL ARCHIVO O DIRECTORIO NO EXISTE
	{
		res = -ENOENT;
	}
	else if (current_file->file_attribute.subdirectory == true) //Si es un directorio
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		; //Libero la memoria de file
	}
	else if (current_file->file_attribute.archive == true) //Si es un archivo
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = current_file->file_size;
		log_debug(log_file,"PFS","fuselage_getattr -> DIRENTRY_getClusterNumber(%s)",current_file->dos_name);
		stbuf->st_ino = DIRENTRY_getClusterNumber(current_file);

		//Libero la memoria de file
	}

	return res;
}

static int fuselage_open(const char *path, struct fuse_file_info *fi)
{
	current_file =  fat32_getDirEntry(path);
	fi->fh = (uint64_t) DIRENTRY_getClusterNumber(current_file);
	return 0;
}

static int fuselage_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	dirEntry_t *file =  fat32_getDirEntry(path);
}
