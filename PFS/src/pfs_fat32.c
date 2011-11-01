/*
 * pfs_fat32.c
 *
 *  Created on: 14/09/2011
 *      Author: utn_so
 */
//TODO Borrar funciones que ya no se usan, crear funcion para liberar un fat32file2_t y una cola de estos
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


char* fat32_getClusterRawData(uint32_t cluster_no)
{
	uint32_t index = 0;
	char* buf = malloc(boot_sector.sectors_perCluster*boot_sector.bytes_perSector);

	uint32_t *sectors = (uint32_t*) cluster_to_sectors(cluster_no);
	uint32_t sector_index;

	for (sector_index = 0; sector_index < 8;sector_index++)
	{
		char* tmp_buf = PFS_sectorRead(*(sectors+sector_index));
		memcpy(buf+(sector_index*boot_sector.bytes_perSector),tmp_buf,boot_sector.bytes_perSector);
		free(tmp_buf);
	}

	free(sectors);
	return buf;
}


cluster_t fat32_getClusterData(uint32_t cluster_number)
{

	cluster_t new_cluster;

	new_cluster.number= cluster_number;
	new_cluster.size = boot_sector.bytes_perSector*boot_sector.sectors_perCluster;
	new_cluster.data = fat32_getClusterRawData(cluster_number);
	uint32_t index_sector;
	uint32_t *sectors = cluster_to_sectors(cluster_number);
	queue_t sector_list;
	QUEUE_initialize(&sector_list);

	for (index_sector = 0;index_sector < boot_sector.sectors_perCluster;index_sector++)
			{
				sector_t *new_sector = malloc(sizeof(sector_t));
				new_sector->data = new_cluster.data+(index_sector*boot_sector.bytes_perSector);
				new_sector->number = *(sectors+index_sector);
				new_sector->size = boot_sector.bytes_perSector;
				new_sector->modified = false;
				QUEUE_appendNode(&sector_list,new_sector);
			}

	free(sectors);
	new_cluster.sectors = sector_list;

	return new_cluster;
}


clusterChain_t fat32_getClusterChainData(uint32_t first_cluster)
{
	queue_t clusterNumber_queue = FAT_getClusterChain(&fat,first_cluster);
	queue_t cluster_list;
	uint32_t bytes_perCluster = boot_sector.sectors_perCluster*boot_sector.bytes_perSector;


	QUEUE_initialize(&cluster_list);
	queueNode_t *cur_clusterNumber_node;

	char* clusterChain_data = malloc(QUEUE_length(&clusterNumber_queue)*bytes_perCluster);
	memset(clusterChain_data,0,QUEUE_length(&clusterNumber_queue)*bytes_perCluster);
	uint32_t cluster_index = 0;

	while ((cur_clusterNumber_node = QUEUE_takeNode(&clusterNumber_queue)) != NULL)
	{
		uint32_t cluster_number = *((uint32_t*) cur_clusterNumber_node->data);
		char* buf = fat32_getClusterRawData(cluster_number);
		memcpy(clusterChain_data+(cluster_index*bytes_perCluster),buf,bytes_perCluster);
		free(buf);
		cluster_t *new_cluster = CLUSTER_newCluster(clusterChain_data+(cluster_index*bytes_perCluster),cluster_number);

		QUEUE_appendNode(&cluster_list,new_cluster);
		free(cur_clusterNumber_node->data);
		free(cur_clusterNumber_node);

		cluster_index++;
	}

	clusterChain_t new_clusterChain;
	new_clusterChain.size = cluster_index;
	new_clusterChain.data = clusterChain_data;
	new_clusterChain.clusters = cluster_list;

	return new_clusterChain;

}

queue_t fat32_readDirectory(const char* path,clusterChain_t *cluster_chain)
{

	char *token;
	bool dir_exists = false;

	*cluster_chain = fat32_getClusterChainData(2);
	queue_t file_list = DIRENTRY_interpretTableData(*cluster_chain);


	if (strcmp(path,"/") == 0) return file_list;

	token = strtok(path,"/");

	queueNode_t* curr_file_node;

	do
	{
		dir_exists = false;
		while ((curr_file_node = QUEUE_takeNode(&file_list)) != NULL)
		{
			fat32file_t *curr_file = (fat32file_t*) curr_file_node->data;
			if (strcmp(curr_file->long_file_name,token) == 0 && (curr_file->dir_entry->file_attribute.subdirectory) == true)
			{

				dir_exists=true;
				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
				uint32_t first_cluster = DIRENTRY_getClusterNumber(curr_file->dir_entry);

				CLUSTER_freeChain(cluster_chain);
				FILE_freeQueue(&file_list);

				*cluster_chain = fat32_getClusterChainData(first_cluster);
				file_list = DIRENTRY_interpretTableData(*cluster_chain);

				break;
			}
			FILE_free(curr_file);
			free(curr_file_node);
		}

		if (dir_exists == false)
		{
			log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x),FAT32FILE_T)",file_list);
			return file_list;
		}

	} while((token = strtok( NULL, "/" )) != NULL && dir_exists == true);

	return file_list;
}

dirEntry_t* fat32_getDirEntry(char* path,clusterChain_t *cluster_chain)
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
	queue_t file_list = fat32_readDirectory(location,cluster_chain); 										//Obtengo una lista de los ficheros que hay en "location"

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
			direntry_found = curr_file->dir_entry;
		}

		log_debug(log_file,"PFS","fat32_getDirEntry() -> LIST_destroyNode(0x%x->data(0x%x)),FAT32FILE_T)",curr_file_node,curr_file_node->data);

		FILE_free(curr_file);
		free(curr_file_node);

		if (direntry_found != NULL) break; 														//Si encontro algo, salgo del ciclo while
	}

	FILE_freeQueue(&file_list);

	return direntry_found;																		//Retorno el puntero a la direntry del archivo buscado
}

fat32file_t fat32_getFileStruct(const char* path)
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

	queue_t file_list; //= fat32_readDirectory(location);
	queueNode_t* cur_node;
	fat32file_t* cur_file;
	fat32file_t ret_file;
	while ((cur_node = QUEUE_takeNode(&file_list)) != NULL)
	{
		cur_file = (fat32file_t*) cur_node->data;
		if (strcmp(cur_file->long_file_name,filename) == 0)
		{

			ret_file = *cur_file;
			ret_file.long_file_name = malloc(strlen(cur_file->long_file_name));
			memset(ret_file.long_file_name,0,strlen(cur_file->long_file_name));
			strcpy(ret_file.long_file_name,cur_file->long_file_name);
		}
		FILE_free(cur_file);
		QUEUE_freeNode(cur_node,0);
	}

	return ret_file;
}

void fat32_writeClusterData(cluster_t *cluster)
{
	queueNode_t *cur_sector_node;
	sector_t *cur_sector;

	while ((cur_sector_node = QUEUE_takeNode(&(cluster->sectors))) != NULL)
	{
		cur_sector = (sector_t*) cur_sector_node->data;
		if (cur_sector->modified) PFS_sectorWrite(*cur_sector);
	}
}

