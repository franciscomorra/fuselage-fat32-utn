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
#include "tad_line.h"
#include "tad_file.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "log.h"
#include <signal.h>
bootSector_t boot_sector;
fatTable_t fat;
t_log *log_file;
dirEntry_t *opened_file;
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

	log_file = log_create("PFS","pfs.log",DEBUG,M_CONSOLE_DISABLE);
	log_debug(log_file,"PFS","Inicio PFS");

	boot_sector.bytes_perSector = 512; //Habra que hacer alguna funcion especial para leer solo el boot_sector;

	fat32_readBootSector(&boot_sector);
	fat32_readFAT(&fat);

	signal(SIGUSR2,cmd_signal);
	return fuse_main(args.argc,args.argv, &fuselage_oper,NULL);
}

int cmd_signal()
{
	pthread_mutex_lock(&signal_lock);

	if (strcmp(cmd_received,"fsinfo") == 0)
	{
			listLine_t* cluster_list = FAT_getFreeClusters(&fat);
			listNode_t* cur = NULL;
			uint32_t count_free = 0;

			while ((cur = LIST_removeFromBegin(&cluster_list)) != NULL)
			{
				++count_free;
				log_debug(log_file,"PFS","%d %x",count_free, cur);
				LIST_destroyNode(&cur,UINT32_T);
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
	listLine_t *file_list = fat32_readDirectory(path); //Obtengo una lista de los ficheros que hay en "path"

	assert(file_list != NULL); //Si falla es que fuselage_getattr no detecto que no era un directorio valido
	listNode_t *curr_file_node = file_list->begin;
	fat32file_t *curr_file;
	while ((curr_file_node = LIST_removeFromBegin(&file_list)) != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;
		filler(buf, curr_file->long_file_name, NULL, 0);
		LIST_destroyNode(&curr_file_node,FAT32FILE_T);
		curr_file_node = curr_file_node->next;
	}

	log_debug(log_file,"PFS","fuselage_readdir -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
	LIST_destroyList(&file_list,FAT32FILE_T);
	//free(file_list);
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
	opened_file = fat32_getDirEntry(path);
	fi->fh = (uint64_t) DIRENTRY_getClusterNumber(opened_file);
	return 0;
}

static int fuselage_read(const char *path, char *file_buf, size_t sizeToRead, off_t offset, struct fuse_file_info *fi)
{

	//size = opened_file->file_size;


	listNode_t* curr_clusterNode = NULL;

	size_t cluster_size_b = boot_sector.sectors_perCluster*boot_sector.bytes_perSector;

	listLine_t* cluster_line = FAT_getClusterChain(&fat,fi->fh);

	uint32_t begin_cluster = offset / cluster_size_b;
	uint32_t end_cluster = (offset+sizeToRead) / cluster_size_b;

	memset(file_buf,0,sizeToRead);

	char* clustersData_buf = malloc((end_cluster - begin_cluster) * cluster_size_b);
	char* tmp_buf =  malloc(cluster_size_b);

	size_t cluster_off = 0;
	size_t clustersData_off = 0;
	while ((curr_clusterNode = LIST_removeFromBegin(&cluster_line)) != NULL)
	{
		if (cluster_off == begin_cluster)
		{
			uint32_t cluster_no = *((uint32_t*) (curr_clusterNode->data));
			memset(tmp_buf,0,cluster_size_b);
			fat32_getClusterData(cluster_no,&tmp_buf);
			memcpy(clustersData_buf+(clustersData_off*cluster_size_b),tmp_buf,cluster_size_b);
			clustersData_off++;
		}
		else if (cluster_off== end_cluster)
		{
			break;
		}
		cluster_off++;
	}

	memcpy(file_buf,clustersData_buf+(offset-(begin_cluster*cluster_size_b)),sizeToRead);
	free(clustersData_buf);
	free(tmp_buf);

	return sizeToRead;


}

static int fuselage_flush(const char *path, struct fuse_file_info *fi)
{
	//NO SE AUN

	return 0;
}
