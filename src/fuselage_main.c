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
#include "tad_queue.h"
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


	//queue_t list = fat32_readDirectory("/");
	//uint32_t santi = 2;
	signal(SIGUSR2,cmd_signal);
	return fuse_main(args.argc,args.argv, &fuselage_oper,NULL);
}

int cmd_signal()
{
	pthread_mutex_lock(&signal_lock);

	if (strcmp(cmd_received,"fsinfo") == 0)
	{
			queue_t cluster_list = FAT_getFreeClusters(&fat);
			queueNode_t* cur = NULL;
			uint32_t count_free = 0;

			while ((cur = QUEUE_takeNode(&cluster_list)) != NULL)
			{
				++count_free;
				log_debug(log_file,"PFS","%d %x",count_free, cur);
				QUEUE_destroyNode(cur,UINT32_T);
				cur = cur->next;
			}

			uint32_t count_notfree = fat.size - count_free;
			//QUEUE_destroy(&cluster_list,UINT32_T);

			printf("\nClusters ocupados: %d\n",count_notfree);
			printf("Clusters libres: %d\n",count_free);
			printf("Tamaño sector: %d bytes\n",boot_sector.bytes_perSector);
			printf("Tamaño cluster: %d bytes\n",(boot_sector.bytes_perSector*boot_sector.sectors_perCluster));
			printf("Tamaño FAT: %d Kb\n\n",(sizeof(uint32_t)*fat.size/1024));
			fflush(stdout);
	}
	//TODO: finfo [path a un archivo] obtener los 20 primeros clusters
	pthread_mutex_unlock(&signal_lock);
	return 0;
}

int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	log_debug(log_file,"PFS","fuselage_readdir -> fat32_readDirectory(%s)",path);
	queue_t file_list = fat32_readDirectory(path); //Obtengo una lista de los ficheros que hay en "path"

	assert(&file_list != NULL); //Si falla es que fuselage_getattr no detecto que no era un directorio valido
	queueNode_t *curr_file_node = file_list.begin;
	fat32file_t *curr_file;
	while ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;
		filler(buf, curr_file->long_file_name, NULL, 0);

		free(curr_file->long_file_name);
		QUEUE_destroyNode(curr_file_node,FAT32FILE_T);
		curr_file_node = curr_file_node->next;
	}

	log_debug(log_file,"PFS","fuselage_readdir -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
	//QUEUE_destroy(&file_list,FAT32FILE_T);
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
	queueNode_t* curr_clusterNode = NULL;

	size_t cluster_size_b = boot_sector.sectors_perCluster*boot_sector.bytes_perSector; 				//Calculo el tamaño en bytes de un cluster

	// = FAT_getClusterChain(&fat,fi->fh);
	queue_t cluster_line = fat32_getClusterChainData(fi->fh);//Obtengo la cadena de clusters asociada al archivo que se leera
	//char* file_content = fat32_getClusterChainRawData(cluster_line)M

	uint32_t begin_cluster = offset / cluster_size_b; 													//Calculo en que cluster cae el offset pedido
	uint32_t end_cluster = (offset+sizeToRead) / cluster_size_b; 										//Calculo en que cluster cae el byte final a leer

	memset(file_buf,0,sizeToRead); 																		//Reservo la cantidad de bytes que se van a leer

	char* clustersData_buf = malloc((end_cluster - begin_cluster) * cluster_size_b); 					//Creo buffer temporal para almacenar todos los clusters del archivo
	//char* tmp_buf =  malloc(cluster_size_b); 															//Creo un buffer temporal para almacenar un cluster

	size_t cluster_off = 0; 																			//Inicializo el offset de clusters a 0
	size_t clustersData_off = 0; 																		//Inicializo el offset dentro de los datos de todos los clusters a 0

	while ((curr_clusterNode = QUEUE_takeNode(&cluster_line)) != NULL) 							//Mientras tomo los nodos que representan todos los clusters del archivo
	{
		if (cluster_off == begin_cluster) 																//Si estoy en el cluster donde empieza el offset
		{
			cluster_t *cur_cluster = (cluster_t*) curr_clusterNode->data;
			//uint32_t cluster_no = *((uint32_t*) (curr_clusterNode->data));								//Guardo el numero de cluster
			//memset(tmp_buf,0,cluster_size_b); 															//Seteo a 0 la memoria donde se guardaran los datos dele cluster en el que estoy
			//fat32_getClusterData(cluster_no,&tmp_buf); 													//Obtengo en el buffer de cluster temporal actual los datos del cluster en el que estoy
			char *tmp_buf = fat32_getClusterRawData(*cur_cluster);
			memcpy(clustersData_buf+(clustersData_off*cluster_size_b),tmp_buf,cluster_size_b);
			free(tmp_buf);//Copio al buffer de todos los clusters estos datos
			clustersData_off++; 																		//Aumento el offset de clusters en 1
		}
		else if (cluster_off == end_cluster) 															//Si estoy en el ultimo cluster necesitado dejo de tomar nodos
		{
			break;
		}
		cluster_off++;
	}

	//QUEUE_destroy(&cluster_line,UINT32_T); 																	//Destruyo la lista de clusters del archivo
	memcpy(file_buf,clustersData_buf+(offset-(begin_cluster*cluster_size_b)),sizeToRead); 				//Copio al buffer final del archivo los bytes pedidos
	free(clustersData_buf); 																			//Libero el buffer que contiene los datos de todos los clusters
	//free(tmp_buf);																						//Libero el buffer que contiene los datos del cluster actual

	return sizeToRead;
}

static int fuselage_flush(const char *path, struct fuse_file_info *fi)
{
	//NO SE AUN

	return 0;
}

static int fuselage_rename(const char *cur_name, const char *new_name)
{
	fat32file_t file_oldName = fat32_getFile(cur_name);
	//uint32_t first_cluster = DIRENTRY_getClusterNumber(file_oldName.dir_entry);


}
