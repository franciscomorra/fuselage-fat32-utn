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
#include "tad_lfnentry.h"
#include "tad_queue.h"
#include "tad_file.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "tad_cluster.h"
#include "tad_sector.h"
#include "file_cache.h"

#include "log.h"
#include <signal.h>
bootSector_t boot_sector;
fatTable_t fat;
t_log *log_file;
dirEntry_t *opened_file;
queue_t file_caches;


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
	FAT_read(&fat);

	signal(SIGUSR2,cmd_signal);
	uint32_t res = fuse_main(args.argc,args.argv, &fuselage_oper,NULL);
	raise(SIGTERM);
	return 0;
}

int cmd_signal()
{
	char* token = (char*) strtok(cmd_received," ");

	if (strcmp(token,"fsinfo") == 0)
	{
			queue_t cluster_list = FAT_getFreeClusters(&fat);
			queueNode_t* cur_cluster_node = NULL;
			uint32_t count_free = 0;

			while ((cur_cluster_node = QUEUE_takeNode(&cluster_list)) != NULL)
			{
				++count_free;
				//QUEUE_freeNode(cur_cluster_node,0);
				free(cur_cluster_node->data);
				free(cur_cluster_node);
			}

			uint32_t count_notfree = fat.size - count_free;
			//QUEUE_destroy(&cluster_list,UINT32_T);

			printf("\nClusters ocupados: %d\n",count_notfree);
			printf("Clusters libres: %d\n",count_free);
			printf("Tamaño sector: %d bytes\n",boot_sector.bytes_perSector);
			printf("Tamaño cluster: %d bytes\n",(boot_sector.bytes_perSector*boot_sector.sectors_perCluster));
			printf("Tamaño FAT: %d Kb\n\n",(sizeof(uint32_t)*fat.size/1024));
			fflush(stdout);
			free(cmd_received);
	}
	else if (strcmp(token,"finfo") == 0)
	{
		token = strtok(NULL," ");

		cluster_set_t cluster_chain;
		memset(&cluster_chain,0,sizeof(cluster_set_t));
		dirEntry_t *direntry = fat32_getDirEntry(token,&cluster_chain);
		if (direntry != NULL)
		{
			printf("#Clusters:\n");
			queue_t cluster_list = FAT_getClusterChain(&fat,DIRENTRY_getClusterNumber(direntry));
			uint32_t cluster_index;
			queueNode_t* cur_node;
			for (cluster_index = 0;cluster_index < 20;cluster_index++)
			{
				if ((cur_node = QUEUE_takeNode(&cluster_list)) != NULL)
				{
					uint32_t *cur_cluster = (uint32_t*) cur_node->data;
					printf("%d, ",*cur_cluster);

					free(cur_node->data);
					free(cur_node);

				}
				else
				{
					printf("\n");
					break;
				}
			}

		}
		else
			printf("No existe el archivo\n");

		CLUSTER_freeChain(&cluster_chain);
	}

	pthread_mutex_unlock(&signal_lock);
	return 0;

}

int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	cluster_set_t cluster_chain;
	memset(&cluster_chain,0,sizeof(cluster_set_t));
	queue_t file_list = fat32_readDirectory(path,&cluster_chain); //Obtengo una lista de los ficheros que hay en "path"

	assert(&file_list != NULL); //Si falla es que fuselage_getattr no detecto que no era un directorio valido
	queueNode_t *curr_file_node = file_list.begin;
	fat32file_t *curr_file;
	while ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;
		filler(buf, curr_file->long_file_name, NULL, 0);

		FILE_free(curr_file);
		free(curr_file_node);
	}
	CLUSTER_freeChain(&cluster_chain);

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
	//log_debug(log_file,"PFS","fuselage_getattr -> fat32_getDirEntry(%s)",path);
	cluster_set_t cluster_chain;
	memset(&cluster_chain,0,sizeof(cluster_set_t));
	dirEntry_t *current_file =  fat32_getDirEntry(path,&cluster_chain); //Obtengo la dirEntry_t del fichero apuntado por path

	if (current_file == NULL) //SI DEVOLVIO NULL, EL ARCHIVO O DIRECTORIO NO EXISTE
	{
		res = -ENOENT;
	}
	else if (current_file->file_attribute.subdirectory == true) //Si es un directorio
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		//free(current_file);
		; //Libero la memoria de file
	}
	else if (current_file->file_attribute.archive == true) //Si es un archivo
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = current_file->file_size;
		log_debug(log_file,"PFS","fuselage_getattr -> DIRENTRY_getClusterNumber(%s)",current_file->dos_name);
		stbuf->st_ino = DIRENTRY_getClusterNumber(current_file);
		//free(current_file);

		//Libero la memoria de file
	}
	CLUSTER_freeChain(&cluster_chain);

	return res;
}

static int fuselage_open(const char *path, struct fuse_file_info *fi)
{
	cluster_set_t cluster_chain;
	memset(&cluster_chain,0,sizeof(cluster_set_t));
	dirEntry_t* opened_file = fat32_getDirEntry(path,&cluster_chain);
	fi->fh = (uint64_t) DIRENTRY_getClusterNumber(opened_file);
	CLUSTER_freeChain(&cluster_chain);
	return 0;
}

static int fuselage_read(const char *path, char *file_buf, size_t sizeToRead, off_t offset, struct fuse_file_info *fi)
{
	cluster_set_t cluster_chain = fat32_readClusterChain(fi->fh);
	memcpy(file_buf,cluster_chain.data+offset,sizeToRead);
	CLUSTER_freeChain(&cluster_chain);
	return strlen(file_buf);
}

static int fuselage_flush(const char *path, struct fuse_file_info *fi)
{
	//Aca se van a grabar en disco los datos guardados en cache!
	//Primero se graban en cache, cuando se necesita espacio en la cache se usa un algoritmo para ver cual eliminar de esta y sino fue grabado se graba y luego se guarda la nueva entrada


	return 0;
}

static int fuselage_rename(const char *cur_name, const char *new_name)
{
	/* INICIO DEFINICION VARIABLE */
		char *new_filename, *new_path, *path,*filename;

		size_t utf16_filename_size = 0;
		char *utf16_filename = malloc(26);
		memset(utf16_filename,0,26);

		cluster_set_t DIRTABLE_cluster_chain;
		memset(&DIRTABLE_cluster_chain,0,sizeof(cluster_set_t));

		queue_t file_list;
		queueNode_t *cur_file_node;

		dirEntry_t* new_file;
	/* FIN DEFINICION VARIABLES */

	/* INICIO LOGICA DE LA FUNCION */
	if (fat32_getDirEntry(new_name,&DIRTABLE_cluster_chain) == NULL) // SI NO EXISTE OTRO ARCHIVO CON EL MISMO NOMBRE
	{
		FILE_splitNameFromPath(new_name,&new_filename,&new_path);
		FILE_splitNameFromPath(cur_name,&filename,&path);

		CLUSTER_freeChain(&DIRTABLE_cluster_chain);
		memset(&DIRTABLE_cluster_chain,0,sizeof(cluster_set_t));
		file_list = fat32_readDirectory(path,&DIRTABLE_cluster_chain);

		cur_file_node = file_list.begin;
		fat32file_t *cur_file;
		while (cur_file_node != NULL)
		{
			cur_file = (fat32file_t*) cur_file_node->data;
			if (strcmp(cur_file->long_file_name,filename) == 0) break;
			cur_file_node = cur_file_node->next;
		}

		LFNENTRY_setNameChars((lfnEntry_t*) cur_file->lfn_entries.begin->data,new_filename);
		DIRENTRY_setDosName(cur_file->dir_entry,new_filename);

		fat32_writeClusterChain(&DIRTABLE_cluster_chain);

		free(new_filename);
		free(new_path);
		free(path);
		free(filename);

		FILE_freeQueue(&file_list);
	}

	free(utf16_filename);
	CLUSTER_freeChain(&DIRTABLE_cluster_chain);
	return 0;

}

static int fuselage_truncate(const char *fullpath, off_t new_size)
{
	char *filename;
	char *path;
	uint32_t bytes_perCluster = boot_sector.bytes_perSector * boot_sector.sectors_perCluster;
	cluster_set_t cluster_chain;
	memset(&cluster_chain,0,sizeof(cluster_set_t));

	dirEntry_t *dir_entry = fat32_getDirEntry(fullpath,&cluster_chain);
	uint32_t original_size = dir_entry->file_size;

	if (original_size == new_size)
	{
		CLUSTER_freeChain(&cluster_chain);
		return 0;
	}

	uint32_t first_cluster_no = DIRENTRY_getClusterNumber(dir_entry);

	queue_t file_clusters = FAT_getClusterChain(&fat,first_cluster_no);
	uint32_t file_clusters_no = QUEUE_length(&file_clusters);
	queueNode_t *file_cluster;
	while((file_cluster = QUEUE_takeNode(&file_clusters)) != NULL) QUEUE_freeNode(file_cluster);


	size_t needed_clusters = ((new_size - 1) / bytes_perCluster) + 1;
	cluster_set_t file_clustersChain;
	char* last_byte;
	if (needed_clusters < file_clusters_no)
	{
		uint32_t rest_to_remove = file_clusters_no - needed_clusters;
		uint32_t index = 0;
		for(index=0;index < rest_to_remove;index++)
		{
				FAT_removeCluster(fat,first_cluster_no);
		}

		file_clustersChain = fat32_readClusterChain(first_cluster_no);
		last_byte = file_clustersChain.data+new_size;
	}
	else if (needed_clusters > file_clusters_no)
	{
		uint32_t index;
		size_t clusters_to_append = needed_clusters - file_clusters_no;

		for(index=0;index < clusters_to_append;index++)
		{
			FAT_appendCluster(fat,first_cluster_no);
		}

		file_clustersChain = fat32_readClusterChain(first_cluster_no);
		last_byte = file_clustersChain.data+original_size;
	}
	else
	{
		file_clustersChain = fat32_readClusterChain(first_cluster_no);
		if (original_size < new_size)
		{
			last_byte = file_clustersChain.data+original_size;
		}
		else if (original_size > new_size)
		{
			last_byte = file_clustersChain.data+new_size;
		}
	}


		uint32_t total_chain_bytes = bytes_perCluster * file_clustersChain.size;

		while (last_byte != file_clustersChain.data+total_chain_bytes)
		{
			*last_byte = '\0';
			last_byte++;
		}

		dir_entry->file_size = new_size;

		sector_t* sect = (sector_t*) fat.sectors.begin->data;
		FAT_write(&fat);
		fat32_writeClusterChain(&cluster_chain);
		fat32_writeClusterChain(&file_clustersChain);
	return 0;
}

static int fuselage_write(const char *fullpath, const char *file_buff, size_t buff_size, off_t off, struct fuse_file_info *fi)
{
	fuselage_truncate(fullpath,buff_size);
	cluster_set_t cluster_chain;
	memset(&cluster_chain,0,sizeof(cluster_set_t));

	dirEntry_t* dir_entry = fat32_getDirEntry(fullpath,&cluster_chain);
	uint32_t cluster_no = DIRENTRY_getClusterNumber(dir_entry);
	CLUSTER_freeChain(&cluster_chain);

	cluster_set_t file_clusters = fat32_readClusterChain(cluster_no);
	memcpy(file_clusters.data+off,file_buff,buff_size);

	fat32_writeClusterChain(&file_clusters);

	CLUSTER_freeChain(&file_clusters);

	return buff_size;
}

static int fuselage_create(const char *fullpath, mode_t mode, struct fuse_file_info *fi)
{
	return fat32_mk(fullpath,ARCHIVE_ATTR);
}

static int fuselage_mkdir(const char *fullpath, mode_t mode)
{
	return fat32_mk(fullpath,DIR_ATTR);

}
