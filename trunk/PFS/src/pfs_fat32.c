/*
 * pfs_fat32.c
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <assert.h>

#include "tad_sector.h"
#include "tad_cluster.h"

#include "pfs_comm.h"
#include "pfs_fat32.h"
#include "utils.h"
#include "tad_direntry.h"
#include "tad_file.h"
#include "tad_queue.h"
#include "tad_lfnentry.h"
#include "log.h"

extern bootSector_t boot_sector;
extern fatTable_t fat;
extern t_log *log_file;

uint32_t fat32_readFAT(fatTable_t *fat)
{

	log_debug(log_file,"PFS","Leyendo FAT Table");
	uint32_t bytes_perFATentry = 4;
	fat->size = (boot_sector.bytes_perSector*boot_sector.sectors_perFat32) / bytes_perFATentry;

	//Luego se reemplazara esto  por el envio del mensaje al PPD/PRAID
	uint32_t sectors[boot_sector.sectors_perFat32];
	int sector;
	for (sector = 32; sector <= 32+boot_sector.sectors_perFat32; sector++)
	{
		sectors[sector-32] = sector;
	}

	fat->table = (uint32_t*) PFS_requestSectorsOperation(READ_SECTORS,sectors,boot_sector.sectors_perFat32);
	assert(*((char*) fat->table) == boot_sector.media_descriptor);
	log_debug(log_file,"PFS","FAT Table OK");
	fat->EOC = *(fat->table + 1);
	return 0;
}


uint32_t fat32_readBootSector(bootSector_t *bs)
{
	log_debug(log_file,"PFS","Leyendo Boot Sector");
	uint32_t sectors[1] = {0} ;
	char *bootsector_data = PFS_requestSectorsOperation(READ_SECTORS,sectors,1);
	assert(*bootsector_data != 0x00);
	log_debug(log_file,"PFS","Boot Sector OK");
	memcpy(bs,bootsector_data,512);
	free(bootsector_data);
	return 0;

}

char* fat32_getClusterRawData(cluster_t cluster)
{
	uint32_t index = 0;
	char* buf = malloc(cluster.size*boot_sector.bytes_perSector);

	queueNode_t *cur_sector_node = ((queue_t) cluster.sectors).begin;
	while (cur_sector_node != NULL)
	{
		sector_t *cur_sector = (sector_t*) cur_sector_node->data;
		memcpy(buf+(index*boot_sector.bytes_perSector),cur_sector->data,cur_sector->size);
		cur_sector_node = cur_sector_node->next;
		index++;

	}
	return buf;
}

char* fat32_getClusterChainRawData(queue_t cluster_list)
{
	size_t bytes_perCluster = boot_sector.bytes_perSector*boot_sector.sectors_perCluster;
	char* buf = malloc(QUEUE_length(&cluster_list)*bytes_perCluster);
	char* tmp_buf;

	uint32_t cluster_index = 0;
	queueNode_t *cur_cluster_node = cluster_list.begin;
	while (cur_cluster_node != NULL)
	{
		cluster_t *cur_cluster = (cluster_t*) cur_cluster_node->data;
		tmp_buf = fat32_getClusterRawData(*cur_cluster);
		memcpy(buf+(cluster_index*bytes_perCluster),tmp_buf,bytes_perCluster);
		free(tmp_buf);
		cur_cluster_node = cur_cluster_node->next;
		cluster_index++;
	}

	return buf;
}

cluster_t fat32_getClusterData(uint32_t cluster_number)
{
	cluster_t new_cluster;
	queue_t clusterNumber_list = FAT_getClusterChain(&fat,cluster_number);
	size_t numberOfClusters = QUEUE_length(&clusterNumber_list);

	queueNode_t *cur_node;

	while  ((cur_node = QUEUE_takeNode(&clusterNumber_list)) != NULL)
	{

		queue_t sector_list;
		QUEUE_initialize(&sector_list);
		uint32_t *sectors = (uint32_t*) cluster_to_sectors(*((uint32_t*) cur_node->data));
		uint32_t index_sector;


		for (index_sector = 0;index_sector < boot_sector.sectors_perCluster;index_sector++)
		{
			sector_t *new_sector = malloc(sizeof(sector_t));
			new_sector->data = PFS_sectorOperation(READ_SECTORS,*(sectors+index_sector));
			new_sector->number = *(sectors+index_sector);
			new_sector->size = boot_sector.bytes_perSector;
			new_sector->modified = false;
			queueNode_t *new_sector_node = QUEUE_createNode(new_sector);
			QUEUE_appendNode(&sector_list,new_sector_node);
		}


		new_cluster.number = *((uint32_t*) cur_node->data);
		new_cluster.sectors = sector_list;
		new_cluster.size = boot_sector.sectors_perCluster;
	}


	return new_cluster;
}

queue_t fat32_getClusterChainData(uint32_t first_cluster)
{
	queue_t clusterNumber_queue = FAT_getClusterChain(&fat,first_cluster);
	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);
	queueNode_t *cur_clusterNumber_node;
	while ((cur_clusterNumber_node = QUEUE_takeNode(&clusterNumber_queue)) != NULL)
	{
		cluster_t *new_cluster = malloc(sizeof(cluster_t));
		*new_cluster = fat32_getClusterData(*((uint32_t*) cur_clusterNumber_node->data));
		queueNode_t *new_cluster_node = QUEUE_createNode(new_cluster);
		QUEUE_appendNode(&cluster_list,new_cluster_node);
	}
	return cluster_list;

}


queue_t fat32_readDirectory(const char* path)
{

	size_t bytes_perCluster = boot_sector.sectors_perCluster*boot_sector.bytes_perSector;
	char *token,*buf;

	bool dir_exists = false;

	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);


	queue_t root_cluster_list = fat32_getClusterChainData(2);

	buf = fat32_getClusterChainRawData(root_cluster_list);
	log_debug(log_file,"PFS","fat32_readDirectory() -> DIRENTRY_interpretDirTableData(0x%x)",buf);
	queue_t file_list = DIRENTRY_interpretDirTableData(buf,bytes_perCluster,2);

	free(buf);

	if (strcmp(path,"/") == 0) return file_list;

	token = strtok(string,"/");

	queueNode_t* curr_file_node;
	queueNode_t *curr_cluster_node;
	do
	{
		dir_exists = false;
		while ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)
		{
			fat32file_t *curr_file = (fat32file_t*) curr_file_node->data;
			if (strcmp(curr_file->long_file_name,token) == 0 && (curr_file->dir_entry.file_attribute.subdirectory) == true)
			{

				dir_exists=true;
				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
				//QUEUE_destroy(&file_list,FAT32FILE_T);

				log_debug(log_file,"PFS","fat32_readDirectory() -> DIRENTRY_getClusterNumber(%s)",curr_file->dir_entry.dos_name);
				uint32_t first_cluster = DIRENTRY_getClusterNumber(&(curr_file->dir_entry));

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_file_node,curr_file_node->data);
				free(curr_file->long_file_name);
				QUEUE_destroyNode(curr_file_node,0);

				//Leo de todos los clusters
				log_debug(log_file,"PFS","fat32_readDirectory() ->  FAT_getClusterChain(0x%x,%d)",&fat,first_cluster);

				queue_t cluster_list = fat32_getClusterChainData(first_cluster);
				char* data_of_clusters = fat32_getClusterChainRawData(cluster_list);
				uint32_t data_of_clusters_size = QUEUE_length(&cluster_list)*bytes_perCluster;

				file_list = DIRENTRY_interpretDirTableData(data_of_clusters,data_of_clusters_size,first_cluster);
				/*
				uint32_t cluster_count = QUEUE_length(&cluster_list);
				char* data_of_clusters = malloc(cluster_count*bytes_perCluster);
				memset(data_of_clusters,0,cluster_count*bytes_perCluster);
				uint32_t cluster_off = 0;
				uint32_t data_of_clusters_size = 0;
				while ((curr_cluster_node = QUEUE_takeNode(&cluster_list)) != NULL)
				{
					fat32_getClusterData(*((uint32_t*) (curr_cluster_node->data)),&buf);
					memcpy(data_of_clusters+(cluster_off*bytes_perCluster),
							buf,
							boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
					data_of_clusters_size += bytes_perCluster;
					free(buf);

					log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_cluster_node,curr_cluster_node->data);
					QUEUE_destroyNode(curr_cluster_node,FAT32FILE_T);
				}*/

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,0)",cluster_list);
				//QUEUE_destroy(&cluster_list,0);

				 //Obtengo la lista de archivos
				log_debug(log_file,"PFS","fat32_readDirectory() -> DIRENTRY_interpretDirTableData(0x%x)",data_of_clusters);

				free(data_of_clusters);
				break;
			}
		}

		if (dir_exists == false)
		{
			log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x),FAT32FILE_T)",file_list);
			//QUEUE_destroy(&file_list,FAT32FILE_T);
			return file_list;
		}

	}

	while((token = strtok( NULL, "/" )) != NULL && dir_exists == true);

	return file_list;

}

dirEntry_t* fat32_getDirEntry(char* path)
{
	size_t len = strlen(path); 																	//Obtengo el largo del path
	char string[len]; 																			//Creo un array para la cadena
	strcpy(string,path); 																		//Copio la cadena al array
	char *tmp = strtok(string,"/"); 															//Obtengo el primer token de la cadena (separando por "/")
	char *filename = tmp; 																		//Guardo el primer token en 'filename'
	while ((tmp = strtok(NULL,"/")) != NULL) 													//Mientras la funcion devuelva un token != NULL guardo ese token en filename
	{
		filename = tmp ; 																		//Al final contendra el filename del archivo que se busca
	}

	size_t location_size = strlen(path) - strlen(filename);										//Calculo el largo del path al archivo (sin incluir el filename del archivo)
	char* location = malloc(location_size+1); 													//Reservo la cantidad necesaria calculada +1 por el caracter '\0'
	memset(location,0,location_size+1); 														//Seteo a 0 la cadena location
	memcpy(location,path,location_size); 														//Copio el path (hasta donde empeiza el filename sin incluirlo) a location
																								//TODO: Primero buscar en cache

	log_debug(log_file,"PFS","fat32_getDirEntry() -> fat32_readDirectory(%s)",location);
	queue_t file_list = fat32_readDirectory(location); 										//Obtengo una lista de los ficheros que hay en "location"

	queueNode_t *curr_file_node; 																//Creo un puntero que apuntara al file_node en proceso
	free(location); 																			//Libero la memoria de location

	fat32file_t *curr_file;																		//Creo un puntero que apuntara al file struct en proceso
	dirEntry_t *direntry_found = NULL;															/* Apunto direntry_found a NULL, si al final de la funcion
																								 sigue en NULL es que no existe el archivo/carpeta */


	while  ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)						//Mientras voy tomando los nodos de la cola
	{
		curr_file = (fat32file_t*) curr_file_node->data; 										//Casteo el puntero 'data' del nodo tomado a el puntero file en proceso

		if (strcmp(curr_file->long_file_name,filename) == 0) 									//Si el nombre conicide con el nombre del archivo buscado es que existe
		{
			direntry_found  = malloc(sizeof(dirEntry_t));
			memcpy(direntry_found,&(curr_file->dir_entry),sizeof(dirEntry_t)); 					//Copio la direntry hacia una estructura a devolver
		}

		log_debug(log_file,"PFS","fat32_getDirEntry() -> LIST_destroyNode(0x%x->data(0x%x)),FAT32FILE_T)",curr_file_node,curr_file_node->data);

		free(curr_file->long_file_name);
		QUEUE_destroyNode(curr_file_node,FAT32FILE_T); 											//Destruyo el nodo que tome de la cola

		if (direntry_found != NULL) break; 														//Si encontro algo, salgo del ciclo while
	}

	//QUEUE_destroy(&file_list,FAT32FILE_T); 													//Destruyo la cola

	return direntry_found;																		//Retorno el puntero a la direntry del archivo buscado
}

fat32file_t fat32_getFile(const char* path)
{
	size_t len = strlen(path); 																	//Obtengo el largo del path
	char string[len]; 																			//Creo un array para la cadena
	strcpy(string,path); 																		//Copio la cadena al array
	char *tmp = strtok(string,"/"); 															//Obtengo el primer token de la cadena (separando por "/")
	char *filename = tmp; 																		//Guardo el primer token en 'filename'
	while ((tmp = strtok(NULL,"/")) != NULL) 													//Mientras la funcion devuelva un token != NULL guardo ese token en filename
	{
		filename = tmp ; 																		//Al final contendra el filename del archivo que se busca
	}

	size_t location_size = strlen(path) - strlen(filename);										//Calculo el largo del path al archivo (sin incluir el filename del archivo)
	char* location = malloc(location_size+1); 													//Reservo la cantidad necesaria calculada +1 por el caracter '\0'
	memset(location,0,location_size+1); 														//Seteo a 0 la cadena location
	memcpy(location,path,location_size);

	queue_t file_list = fat32_readDirectory(location);
	queueNode_t* cur_node;
	fat32file_t* cur_file;
	fat32file_t ret_file;
	while ((cur_node = QUEUE_takeNode(&file_list)) != NULL)
	{
		cur_file = (fat32file_t*) cur_node->data;
		if (strcmp(cur_file->long_file_name,filename) == 0)
		{
			ret_file = *cur_file;
		}
		QUEUE_destroyNode(cur_node,0);
	}

	return ret_file;
}

void fat32_destroyClusterQueue(queue_t *queue)
{

}
