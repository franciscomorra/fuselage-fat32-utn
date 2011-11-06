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


char* fat32_readRawCluster(uint32_t cluster_no)
{
	uint32_t index = 0;
	char* buf = malloc(boot_sector.sectors_perCluster*boot_sector.bytes_perSector);

	uint32_t *sectors = (uint32_t*) cluster_to_sectors(cluster_no);
	uint32_t sector_index;

	for (sector_index = 0; sector_index < 8;sector_index++)
	{
		char* tmp_buf = PPDINTERFACE_readSector(*(sectors+sector_index)); //TODO: ARMAR ARRAY DE SECTORES Y MANDARLOS
		memcpy(buf+(sector_index*boot_sector.bytes_perSector),tmp_buf,boot_sector.bytes_perSector);
		free(tmp_buf);
	}

	free(sectors);
	return buf;
}


cluster_t fat32_readCluster(uint32_t cluster_number)
{

	cluster_t new_cluster;

	new_cluster.number= cluster_number;
	new_cluster.size = boot_sector.bytes_perSector*boot_sector.sectors_perCluster;
	new_cluster.data = fat32_readRawCluster(cluster_number);
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


clusterChain_t fat32_readClusterChain(uint32_t first_cluster)
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
		char* buf = fat32_readRawCluster(cluster_number);
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

queue_t fat32_readDirectory(char* path,clusterChain_t *cluster_chain)
{

	if (path[strlen(path)-1] == '/' && strlen(path) != 1)
	{
		path[strlen(path)-1] = '\0';
	}

	char *token;
	bool dir_exists = false;

	if ((*cluster_chain).clusters.begin == NULL)
	*cluster_chain = fat32_readClusterChain(2);
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

				*cluster_chain = fat32_readClusterChain(first_cluster);
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
	char *location;
	char *filename;																					//Copio el path (hasta donde empeiza el filename sin incluirlo) a location
																								//TODO: Primero buscar en cache
	FILE_splitNameFromPath(path,&filename,&location);
	//log_debug(log_file,"PFS","fat32_getDirEntry() -> fat32_readDirectory(%s)",location);
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

uint32_t fat32_mk(char* fullpath,uint32_t dir_or_archive) //0 o 1
{
		char *filename;
		char *path;
		lfnEntry_t new_lfn;
		dirEntry_t new_direntry;
		FILE_splitNameFromPath(fullpath,&filename,&path);

		clusterChain_t cluster_chain;
		memset(&cluster_chain,0,sizeof(clusterChain_t));
		uint32_t folderTableCluster;
		if (strcmp(path,"/") != 0)
		{
			dirEntry_t *entryOfFolder = fat32_getDirEntry(path,&cluster_chain);
			folderTableCluster = DIRENTRY_getClusterNumber(entryOfFolder);
			CLUSTER_freeChain(&cluster_chain);
		}
		else
		{
			folderTableCluster = 2;
		}
		clusterChain_t dirtable = fat32_readClusterChain(folderTableCluster);


		dirEntry_t *dirtable_index = (dirEntry_t*) dirtable.data;
		dirEntry_t *lastPosForAnEntry = (dirEntry_t*) (dirtable.data+(dirtable.size*boot_sector.bytes_perSector*boot_sector.sectors_perCluster)-sizeof(dirEntry_t));
		bool write = false, freeSpace = false;
		uint32_t entry_offset = 0;


		while (dirtable_index != lastPosForAnEntry)
		{
			if (*((char*) dirtable_index) == 0xE5 || *((char*) dirtable_index) == 0x00)
			{
				write = freeSpace = true;
				break;
			}
			entry_offset++;
			dirtable_index++;
		}

		if (freeSpace == false)
		{
			write = true;
			CLUSTER_freeChain(&dirtable);
			FAT_appendCluster(fat,folderTableCluster);
			dirtable = fat32_readClusterChain(folderTableCluster);
		}

		queueNode_t *cluster_node = dirtable.clusters.begin;

		while (cluster_node != NULL)
		{
			if (cluster_node->next == NULL) break;
			cluster_node = cluster_node->next;
		}
		cluster_t* last_cluster = (cluster_t*) cluster_node->data;
		uint32_t cluster_toAssign = FAT_getFreeCluster(&fat);
		FAT_setUsed(&fat,cluster_toAssign);

		new_lfn = LFNENTRY_create(filename);
		new_direntry = DIRENTRY_create(filename,cluster_toAssign,dir_or_archive);

		memcpy(last_cluster->data+(entry_offset*sizeof(lfnEntry_t)),&new_lfn,sizeof(lfnEntry_t));
		memcpy(last_cluster->data+((entry_offset+1)*sizeof(lfnEntry_t)),&new_direntry,sizeof(dirEntry_t));

		if (write)
		{
			fat32_writeClusterChain(&dirtable);
			FAT_write(&fat);
		}
		CLUSTER_freeChain(&dirtable);
		return 0;
}
/*fat32file_t fat32_getFileStruct(const char* path,clusterChain_t* cluster_chain)
{
	char *location;
	char *filename;
	FILE_splitNameFromPath(path,&filename,&location);

	queue_t file_list = fat32_readDirectory(location,cluster_chain);
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
		free(cur_node);
	}
	free(filename);
	free(location);
	return ret_file;
}*/

void fat32_writeCluster(cluster_t *cluster)
{
	queueNode_t *cur_sector_node;

	while ((cur_sector_node = QUEUE_takeNode(&(cluster->sectors))) != NULL)
	{
		PPDINTERFACE_writeSector(*((sector_t*) cur_sector_node->data));
		QUEUE_freeNode(cur_sector_node);
	}
}

void fat32_writeClusterChain(clusterChain_t* cluster_chain)
{
	queueNode_t *cur_cluster_node;

	while ((cur_cluster_node = QUEUE_takeNode(&(cluster_chain->clusters))) != NULL)
	{
		fat32_writeCluster((cluster_t*) cur_cluster_node->data);
		QUEUE_freeNode(cur_cluster_node);
	}
}
