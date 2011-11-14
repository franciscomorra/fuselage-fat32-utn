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
#include <math.h>
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
		fat32file_2_t *fileentry = fat32_getFileEntry(token);
		if (fileentry != NULL)
		{
			printf("#Clusters:\n");
			queue_t cluster_list = FAT_getClusterChain(&fat,DIRENTRY_getClusterNumber(&fileentry->dir_entry));
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
	queue_t file_list = fat32_readDirectory2(path); //Obtengo una lista de los ficheros que hay en "path"
	queueNode_t *curr_file_node = file_list.begin;
	fat32file_t *curr_file;
	while ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;
		filler(buf, curr_file->long_file_name, NULL, 0);

		FILE_free(curr_file);
		free(curr_file_node);
	}
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

	fat32file_2_t *current_file =  fat32_getFileEntry(path); //Obtengo la dirEntry_t del fichero apuntado por path

	if (current_file == NULL) //SI DEVOLVIO NULL, EL ARCHIVO O DIRECTORIO NO EXISTE
	{
		res = -ENOENT;
	}
	else if (current_file->dir_entry.file_attribute.subdirectory == true) //Si es un directorio
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		//free(current_file);
		; //Libero la memoria de file
	}
	else if (current_file->dir_entry.file_attribute.archive == true) //Si es un archivo
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = current_file->dir_entry.file_size;
		log_debug(log_file,"PFS","fuselage_getattr -> DIRENTRY_getClusterNumber(%s)",current_file->dir_entry.dos_name);
		stbuf->st_ino = DIRENTRY_getClusterNumber(current_file);

	}
	//if (strcmp(path,"prueba") == 0) return 0;
	return res;
}

static int fuselage_open(const char *path, struct fuse_file_info *fi)
{
	fat32file_2_t* opened_file = fat32_getFileEntry(path);
	fi->fh = (uint64_t) DIRENTRY_getClusterNumber(&opened_file->dir_entry);
	//queue_t FAT_getClusterChain(fi->fh); LISTA DE CLUSTERS DEL ARCHIVO ABIERTO, LOGGEAR

	return 0;
}

static int fuselage_read(const char *path, char *file_buf, size_t sizeToRead, off_t offset, struct fuse_file_info *fi)
{
	queue_t cluster_list = FAT_getClusterChain(&fat,fi->fh);
	queueNode_t *cluster = cluster_list.begin;
	uint32_t cluster_count = 0;
	char* tmp_buf = malloc(QUEUE_length(&cluster_list)*4096);
	while (cluster != NULL)
	{
		cache_block_t* block;
		if ((block =CACHE_readFile(&file_caches,path,*((uint32_t*) cluster->data))) != NULL)
		{
			memcpy(tmp_buf+(cluster_count*4096),block->data,4096);
		}
		else
		{
			cluster_t cluster_data = fat32_readCluster(*((uint32_t*) cluster->data));
			CACHE_writeFile(&file_caches,path,cluster_data);
			memcpy(tmp_buf+(cluster_count*4096),cluster_data.data,4096);

		}
		cluster = cluster->next;
	}

	memcpy(file_buf,tmp_buf+offset,sizeToRead);
	free(tmp_buf);

	return strlen(file_buf);
}

static int fuselage_flush(const char *path, struct fuse_file_info *fi)
{

	queueNode_t *cur_cache_node;
	queue_t aux_queue;
	QUEUE_initialize(&aux_queue);

	while ((cur_cache_node = QUEUE_takeNode(&file_caches)) != NULL)
	{
		file_cache_t *cur_cache = (file_cache_t*) cur_cache_node->data;
		if (strcmp(cur_cache->path,path) == 0)
		{
			queueNode_t* cur_cache_block;
			while((cur_cache_block = QUEUE_takeNode(&cur_cache->blocks)) != NULL)
			{
				cache_block_t *cur_block = (cache_block_t*) cur_cache_block->data;
				cluster_t *write_cluster = CLUSTER_newCluster(cur_block->data,cur_block->cluster_no);
				fat32_writeCluster(write_cluster);
				CLUSTER_free(write_cluster);
				free(write_cluster);

				free(cur_block);
				free(cur_cache_block);
			}
			free(cur_cache->path);
			free(cur_cache);
			free(cur_cache_node);
		}
		else
		{
			QUEUE_appendNode(&aux_queue,cur_cache);
		}
	}

	file_caches = aux_queue;
	return 0;
}

static int fuselage_rename(const char *cur_name, const char *new_name)
{
	/* INICIO DEFINICION VARIABLE */
		char *new_filename, *new_path, *path,*filename;
		size_t utf16_filename_size = 0;
		char *utf16_filename = malloc(26);
		memset(utf16_filename,0,26);
	/* FIN DEFINICION VARIABLES */

	/* INICIO LOGICA DE LA FUNCION */

	if (fat32_getFileEntry(new_name) == NULL) // SI NO EXISTE OTRO ARCHIVO CON EL MISMO NOMBRE
	{
		FILE_splitNameFromPath(new_name,&new_filename,&new_path);
		FILE_splitNameFromPath(cur_name,&filename,&path);

		if (strcmp(path,new_path) == 0)
		{
			fat32file_2_t *file_entry = fat32_getFileEntry(cur_name);

			LFNENTRY_setNameChars(&file_entry->lfn_entry,new_filename);
			DIRENTRY_setDosName(&file_entry->dir_entry,new_filename);

			cluster_t entry_cluster = fat32_readCluster(file_entry->cluster);

			memcpy(entry_cluster.data+file_entry->offset,&file_entry->lfn_entry,sizeof(lfnEntry_t));
			memcpy(entry_cluster.data+file_entry->offset+sizeof(lfnEntry_t),&file_entry->dir_entry,sizeof(dirEntry_t));

			free(new_filename);
			free(new_path);
			free(path);
			free(filename);

			fat32_writeCluster(&entry_cluster);
		}
		else
		{
			fat32file_2_t *file_entry = fat32_getFileEntry(cur_name);

			LFNENTRY_setNameChars(&file_entry->lfn_entry,new_filename);
			DIRENTRY_setDosName(&file_entry->dir_entry,new_filename);

			fat32file_2_t *newpath_entry = fat32_getFileEntry(new_path);

			cluster_t path_cluster = fat32_readCluster(file_entry->cluster);
			cluster_t newpath_cluster = fat32_readCluster(DIRENTRY_getClusterNumber(&newpath_entry->dir_entry));

			*(path_cluster.data+file_entry->offset) = 0xE5;
			*(path_cluster.data+file_entry->offset+sizeof(dirEntry_t)) = 0xE5;

			fat32_writeCluster(&path_cluster);

			dirEntry_t *dirtable_index = (dirEntry_t*) newpath_cluster.data;
			dirEntry_t *dirtable_lastentry = (dirEntry_t*) (newpath_cluster.data + 4096 - sizeof(dirEntry_t));
			while (dirtable_index != dirtable_lastentry)
			{
				if (*((char*) dirtable_index) == 0xE5 || *((char*) dirtable_index) == 0x00)
				{
					memcpy(dirtable_index,&file_entry->lfn_entry,sizeof(dirEntry_t));
					memcpy(dirtable_index+sizeof(dirEntry_t),&file_entry->dir_entry,sizeof(dirEntry_t));
					break;
				}
			}

			fat32_writeCluster(&newpath_cluster);

		}

	}

	free(utf16_filename);

	return 0;

}

static int fuselage_truncate(const char *fullpath, off_t new_size)
{
	return fat32_truncate(fullpath,new_size);
}

static int fuselage_write(const char *fullpath, const char *file_buff, size_t buff_size, off_t off, struct fuse_file_info *fi)
{
	fat32_truncate(fullpath,off+buff_size);
	fat32file_2_t *file_entry = fat32_getFileEntry(fullpath);
	uint32_t first_cluster_no = DIRENTRY_getClusterNumber(&file_entry->dir_entry);
	queue_t file_clusters = FAT_getClusterChain(&fat,first_cluster_no);

	uint32_t bytes_perCluster = boot_sector.bytes_perSector * boot_sector.sectors_perCluster;

	uint32_t first_cluster_write = off / bytes_perCluster;
	uint32_t offset_in_cluster = off % bytes_perCluster;

	//uint32_t clusters_in_write = ceilf(((float) buff_size) / ((float) 4096));

	uint32_t cluster_index = 0;
	//SACO TODOS LOS NODOS HATA EL PRIMER CLUSTER A ESCRIBIR
	for (;cluster_index < first_cluster_write;cluster_index++)
	{
			queueNode_t *cluster_node = QUEUE_takeNode(&file_clusters);
			free(cluster_node->data);
			free(cluster_node);
	}

	queueNode_t *cluster_node = QUEUE_takeNode(&file_clusters);
	uint32_t cluster_no = *((uint32_t*) cluster_node->data);
	cluster_t cluster = fat32_readCluster(cluster_no);
	cluster_t *cl;
	memcpy(cluster.data+offset_in_cluster,file_buff,buff_size);
	if ((cl = CACHE_writeFile(&file_caches,fullpath,cluster)) != NULL)
	{
		fat32_writeCluster(cl);
	}
	else
	{
		fat32_writeCluster(&cluster);
	}
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

static int fuselage_rmdir(const char *path)
{
	fat32_remove(path);
	return 0;
}

static int fuselage_unlink(const char *path)
{
	fat32_remove(path);
	return 0;
}
