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

#include "log.h"
#include <signal.h>
bootSector_t boot_sector;
fatTable_t fat;
t_log *log_file;
dirEntry_t *opened_file;


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
	//MUTEX, ARREGLAR!!!!!
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

		clusterChain_t cluster_chain;
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
	clusterChain_t cluster_chain;
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
	log_debug(log_file,"PFS","fuselage_getattr -> fat32_getDirEntry(%s)",path);
	clusterChain_t cluster_chain;
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
	clusterChain_t cluster_chain;
	dirEntry_t* opened_file = fat32_getDirEntry(path,&cluster_chain);
	fi->fh = (uint64_t) DIRENTRY_getClusterNumber(opened_file);
	CLUSTER_freeChain(&cluster_chain);
	return 0;
}

static int fuselage_read(const char *path, char *file_buf, size_t sizeToRead, off_t offset, struct fuse_file_info *fi)
{
	clusterChain_t cluster_chain = fat32_getClusterChainData(fi->fh);
	memcpy(file_buf,cluster_chain.data+offset,sizeToRead);
	CLUSTER_freeChain(&cluster_chain);
	return sizeToRead;
}

static int fuselage_flush(const char *path, struct fuse_file_info *fi)
{
	//NO SE AUN


	return 0;
}

static int fuselage_rename(const char *cur_name, const char *new_name)
{
	size_t len = strlen(cur_name); 																	//Obtengo el largo del path
	char string[len]; 																			//Creo un array para la cadena
	strcpy(string,cur_name); 																		//Copio la cadena al array
	char *tmp = strtok(string,"/"); 															//Obtengo el primer token de la cadena (separando por "/")
	char *filename = tmp; 																		//Guardo el primer token en 'filename'
	while ((tmp = strtok(NULL,"/")) != NULL) 													//Mientras la funcion devuelva un token != NULL guardo ese token en filename
	{
		filename = tmp ; 																		//Al final contendra el filename del archivo que se busca
	}

	size_t location_size = strlen(cur_name) - strlen(filename);										//Calculo el largo del path al archivo (sin incluir el filename del archivo)
	char* location = malloc(location_size+1); 													//Reservo la cantidad necesaria calculada +1 por el caracter '\0'
	memset(location,0,location_size+1); 														//Seteo a 0 la cadena location
	memcpy(location,cur_name,location_size);

	if (strcmp(location,"/") == 0)
	{

	}
	else
	{

	}
	/*
	fat32file_t location_file = fat32_getFileStruct(cur_name);
	cluster_t clust = fat32_getClusterData2(location_file.clusterofEntry);
	queue_t file_list = DIRENTRY_interpretDirTableData2(clust.data,4096,location_file.clusterofEntry);
	queueNode_t *file_node = file_list.begin;
	while (file_node != NULL)
	{
		fat32file2_t* file = file_node->data;
		queueNode_t *lfn_node = file->lfn_entries.begin;
		while (lfn_node != NULL)
		{
			lfnEntry_t *lfn = (lfnEntry_t*) lfn_node->data;

			lfn->sequence_no.deleted = true;
			lfn->sequence_no.last = true;
			lfn->sequence_no.number = 37;

			lfn_node = lfn_node->next;
		}
		file_node = file_node->next;
	}*/

	//Obtener lista de clusters, ver en que cluster esta la entrada, modificar el cluster, escribirlo



}
