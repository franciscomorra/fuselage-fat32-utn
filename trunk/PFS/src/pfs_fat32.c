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

#include "pfs_comm.h"
#include "pfs_fat32.h"
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

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf)
{
	uint32_t *sectors = (uint32_t*) cluster_to_sectors(cluster_no);
	*buf = PFS_requestSectorsOperation(READ_SECTORS,sectors,8);
	/*log_debug(log_file,"PFS","Leyendo Cluster %d (Sectores: %d %d %d %d %d %d %d %d)",cluster_no,
	sectors[0],sectors[1],sectors[2],sectors[3],sectors[4],sectors[5],sectors[6],sectors[7]);*/
	free(sectors);
	return 0;
}

queue_t* fat32_readDirectory(const char* path)
{
	char *token,*buf;
	bool dir_exists = false;
	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);
	log_debug(log_file,"PFS","fat32_readDirectory() -> fat32_getClusterData(2,0x%x)",buf);
	fat32_getClusterData(2,&buf);
	log_debug(log_file,"PFS","fat32_readDirectory() -> getFileList(0x%x)",buf);
	queue_t* file_list = DIRENTRY_interpretDirTableData(buf);

	free(buf);
	if (strcmp(path,"/") == 0) return file_list;


	token = strtok(string,"/");

	queueNode_t* curr_file_node;
	queueNode_t *curr_cluster_node;
	do
	{
		dir_exists = false;
		while ((curr_file_node = QUEUE_removeFromBegin(&file_list)) != NULL)
		{
			fat32file_t *curr_file = (fat32file_t*) curr_file_node->data;
			if (strcmp(curr_file->long_file_name,token) == 0 && (curr_file->dir_entry.file_attribute.subdirectory) == true)
			{
				dir_exists=true;
				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
				QUEUE_destroy(&file_list,FAT32FILE_T);

				log_debug(log_file,"PFS","fat32_readDirectory() -> DIRENTRY_getClusterNumber(%s)",curr_file->dir_entry.dos_name);
				uint32_t first_cluster = DIRENTRY_getClusterNumber(&(curr_file->dir_entry));

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_file_node,curr_file_node->data);
				QUEUE_destroyNode(&curr_file_node,0);

				//Leo de todos los clusters
				log_debug(log_file,"PFS","fat32_readDirectory() ->  FAT_getClusterChain(0x%x,%d)",&fat,first_cluster);

				queue_t *cluster_list = FAT_getClusterChain(&fat,first_cluster);

				uint32_t cluster_count = QUEUE_length(&cluster_list);
				char* data_of_clusters = malloc(cluster_count*boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
				memset(data_of_clusters,0,cluster_count*boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
				uint32_t cluster_off = 0;

				while ((curr_cluster_node = QUEUE_removeFromBegin(&cluster_list)) != NULL)
				{
					fat32_getClusterData(*((uint32_t*) (curr_cluster_node->data)),&buf);
					memcpy(data_of_clusters+(cluster_off*boot_sector.sectors_perCluster*boot_sector.bytes_perSector),
							buf,
							boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
					free(buf);

					log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_cluster_node,curr_cluster_node->data);
					QUEUE_destroyNode(&curr_cluster_node,FAT32FILE_T);
				}

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,0)",cluster_list);
				QUEUE_destroy(&cluster_list,0);

				 //Obtengo la lista de archivos
				log_debug(log_file,"PFS","fat32_readDirectory() -> fat32_getFileList(0x%x)",data_of_clusters);
				file_list = DIRENTRY_interpretDirTableData(data_of_clusters);
				free(data_of_clusters);
				break;
			}
		}

		if (dir_exists == false)
		{
			log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x),FAT32FILE_T)",file_list);
			QUEUE_destroy(&file_list,FAT32FILE_T);
			return NULL;
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
	queue_t *file_list = fat32_readDirectory(location); 										//Obtengo una lista de los ficheros que hay en "location"

	queueNode_t *curr_file_node; 																//Creo un puntero que apuntara al file_node en proceso
	free(location); 																			//Libero la memoria de location

	fat32file_t *curr_file;																		//Creo un puntero que apuntara al file struct en proceso
	dirEntry_t *direntry_found = NULL;															/* Apunto direntry_found a NULL, si al final de la funcion
																								 sigue en NULL es que no existe el archivo/carpeta */


	while  ((curr_file_node = QUEUE_removeFromBegin(&file_list)) != NULL)						//Mientras voy tomando los nodos de la cola
	{
		curr_file = (fat32file_t*) curr_file_node->data; 										//Casteo el puntero 'data' del nodo tomado a el puntero file en proceso

		if (strcmp(curr_file->long_file_name,filename) == 0) 									//Si el nombre conicide con el nombre del archivo buscado es que existe
		{
			direntry_found  = malloc(sizeof(dirEntry_t));
			memcpy(direntry_found,&(curr_file->dir_entry),sizeof(dirEntry_t)); 					//Copio la direntry hacia una estructura a devolver
		}

		log_debug(log_file,"PFS","fat32_getDirEntry() -> LIST_destroyNode(0x%x->data(0x%x)),FAT32FILE_T)",curr_file_node,curr_file_node->data);
		QUEUE_destroyNode(&curr_file_node,FAT32FILE_T); 											//Destruyo el nodo que tome de la cola

		if (direntry_found != NULL) break; 														//Si encontro algo, salgo del ciclo while
	}

	QUEUE_destroy(&file_list,FAT32FILE_T); 													//Destruyo la cola

	return direntry_found;																		//Retorno el puntero a la direntry del archivo buscado
}
