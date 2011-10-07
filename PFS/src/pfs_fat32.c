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
#include "tad_line.h"
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

listLine_t* fat32_readDirectory(const char* path)
{
	char *token,*buf;
	bool dir_exists = false;
	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);
	log_debug(log_file,"PFS","fat32_readDirectory() -> fat32_getClusterData(2,0x%x)",buf);
	fat32_getClusterData(2,&buf);
	log_debug(log_file,"PFS","fat32_readDirectory() -> getFileList(0x%x)",buf);
	listLine_t* file_list = fat32_getFileList(buf);

	free(buf);
	if (strcmp(path,"/") == 0) return file_list;


	token = strtok(string,"/");

	listNode_t* curr_file_node;
	listNode_t *curr_cluster_node;
	do
	{
		dir_exists = false;
		while ((curr_file_node = LIST_removeFromBegin(&file_list)) != NULL)
		{
			fat32file_t *curr_file = (fat32file_t*) curr_file_node->data;
			if (strcmp(curr_file->long_file_name,token) == 0 && (curr_file->dir_entry.file_attribute.subdirectory) == true)
			{
				dir_exists=true;
				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,FAT32FILE_T)",file_list);
				LIST_destroyList(&file_list,FAT32FILE_T);

				log_debug(log_file,"PFS","fat32_readDirectory() -> DIRENTRY_getClusterNumber(%s)",curr_file->dir_entry.dos_name);
				uint32_t first_cluster = DIRENTRY_getClusterNumber(&(curr_file->dir_entry));

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_file_node,curr_file_node->data);
				LIST_destroyNode(&curr_file_node,0);

				//Leo de todos los clusters
				log_debug(log_file,"PFS","fat32_readDirectory() ->  FAT_getClusterChain(0x%x,%d)",&fat,first_cluster);
				listNode_t *cluster_list = FAT_getClusterChain(&fat,first_cluster);

				uint32_t cluster_count = LIST_listSize(&cluster_list);
				char* data_of_clusters = malloc(cluster_count*boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
				memset(data_of_clusters,0,cluster_count*boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
				uint32_t cluster_off = 0;

				while ((curr_cluster_node = LIST_removeFromBegin(&cluster_list)) != NULL)
				{
					fat32_getClusterData(*((uint32_t*) (curr_cluster_node->data)),&buf);
					memcpy(data_of_clusters+(cluster_off*boot_sector.sectors_perCluster*boot_sector.bytes_perSector),
							buf,
							boot_sector.sectors_perCluster*boot_sector.bytes_perSector);
					free(buf);

					log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyNode(0x%x->data(0x%x),FAT32FILE_T)",curr_cluster_node,curr_cluster_node->data);
					LIST_destroyNode(&curr_cluster_node,FAT32FILE_T);
				}

				log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x,0)",cluster_list);
				LIST_destroyList(&cluster_list,0);

				 //Obtengo la lista de archivos
				log_debug(log_file,"PFS","fat32_readDirectory() -> fat32_getFileList(0x%x)",data_of_clusters);
				file_list = fat32_getFileList(data_of_clusters);
				free(data_of_clusters);
				break;
			}
		}

		if (dir_exists == false)
		{
			log_debug(log_file,"PFS","fat32_readDirectory() -> LIST_destroyList(0x%x),FAT32FILE_T)",file_list);
			LIST_destroyList(&file_list,FAT32FILE_T);
			return NULL;
		}

	}
	while((token = strtok( NULL, "/" )) != NULL && dir_exists == true);

	return file_list;
}

listLine_t* fat32_getFileList(char* cluster_data) {
	lfnEntry_t *lfn_entry = (lfnEntry_t*) cluster_data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero
	char* longfilename_buf = malloc(255); //Uso este buffer para ir almacenando los LFN de un archivo
	memset(longfilename_buf, 0, 255); //Lo seteo a 0
	char *tmp_longfilename_part, *new_longfilename; //Puntero para UN LFN de UN archivo, y puntero para el nombre largo completo de un archivo
	size_t tmp_longfilename_part_size = 0, new_longfilename_size = 0; //Tamaño de cadena de los punteros
	listNode_t *new_file_node; //Punteros a nodos de una lista que sera la que se obtenga de esta funcion
	listLine_t *file_list;
	LIST_initialize(&file_list);

	if (*cluster_data == '.')
	{
		dirEntry_t *point = (dirEntry_t*) lfn_entry;
		fat32file_t *pointFile = FILE_createStruct(".",point);
		new_file_node = LIST_createNode(pointFile);
		LIST_addNode(&file_list,&new_file_node);
		lfn_entry++;

		dirEntry_t *pointpoint = (dirEntry_t*) (lfn_entry);
		fat32file_t *pointpointFile = FILE_createStruct("..",pointpoint);
		new_file_node = LIST_createNode(pointpointFile);
		LIST_addNode(&file_list,&new_file_node);
		++lfn_entry;
	}


	while (*((char*) lfn_entry) != 0x00) //Mientras el primer byte de cada 32 bytes que voy recorriendo sea distinto de 0x00 quiere decir que hay una LFN o una DIRENTRY
	{
		//Borrado : number = 37
		if (lfn_entry->sequence_no.number == 1 && lfn_entry->sequence_no.deleted == false) //Si es la ultima LFN del archivo y no esta eliminada (Saltea tambien la DIRENTRY ya que las marca igual que las LFN eliminadas)
		{

			tmp_longfilename_part_size = LFNENTRY_getLongFileName(*lfn_entry,&tmp_longfilename_part); //Obtengo la ultima parte (Viene a ser la primera del nombre , estan dispuestas al reves)
			new_longfilename_size += tmp_longfilename_part_size; //Aumento el tamaño del nombre para obtener el tamaño final
			/* Corro lo que esta almacenado en el buffer
			 * la cantidad de posiciones necesarias hacia la derecha para que entre
			 * la ultima parte del nombre
			 * */

			shiftbytes_right(longfilename_buf, 255, tmp_longfilename_part_size);
			/* Copio la siguiente parte del nombre en el buffer donde
			 * quedara el nombre completo
			 */
			memcpy(longfilename_buf, tmp_longfilename_part,	tmp_longfilename_part_size);
			new_longfilename = malloc(new_longfilename_size + 1); /* Obtengo memoria para el nuevo nombre de archivo */
			memset(new_longfilename, 0, new_longfilename_size + 1); // Seteo a 0
			memcpy(new_longfilename, longfilename_buf, new_longfilename_size); /*Copio el nombre completo que esta en el buffer a la memoria almacenada antes */
			memset(longfilename_buf, 0, 255); /* Seteo el buffer donde se van almacenando las partes a 0 para empezar con el siguiente archivo */
			free(tmp_longfilename_part); //Libero memoria

			/* Aca empiLOG_DEBUG(eza a leer la DIRENTRY del archivo (se podria cambiar por una funcion que cree el nodo) */
			dirEntry_t *direntry = (dirEntry_t*) ++lfn_entry;

			fat32file_t *new_file = FILE_createStruct(new_longfilename,direntry);
			listNode_t *new_file_node = LIST_createNode(new_file);
			LIST_addNode(&file_list,&new_file_node);

			/* Aca termina de leer la DIRENTRY del archivo */
			lfn_entry++; // Apunto al primer LFN del siguiente archivo

			free(new_longfilename);

			new_longfilename_size = 0;

		}
		else if (lfn_entry->sequence_no.number != 1 && lfn_entry->sequence_no.deleted == false) //Si no es la ultima LFN del archivo
		{
			tmp_longfilename_part_size = LFNENTRY_getLongFileName(*lfn_entry, &tmp_longfilename_part); //Obtengo la cadena parte del nombre del LFN
			new_longfilename_size += tmp_longfilename_part_size; //Aumento el tamaño del nombre del archivo que estoy leyendo
			/* Corro lo que esta almacenado en el buffer
			 * la cantidad de posiciones necesarias hacia la derecha para que entre
			 * la siguiente parte del nombre
			 * */
			shiftbytes_right(longfilename_buf, 255, tmp_longfilename_part_size);
			/* Copio la siguiente parte del nombre en el buffer donde
			 * voy uniendo las partes
			 */
			memcpy(longfilename_buf, tmp_longfilename_part,	tmp_longfilename_part_size);
			free(tmp_longfilename_part); //Libero la memoria usada
			lfn_entry++; //Paso a la siguiente entrada LFN
		}
		else if (lfn_entry->sequence_no.deleted == true) //Si es una entrada eliminada la salteo
		{
			lfn_entry++;
		}

	}

	free(longfilename_buf);

	return file_list; //Retorno un puntero al primer FILE_NODE
}

dirEntry_t* fat32_getDirEntry(char* path)
{
	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);
	char *tmp = strtok(string,"/");
	char *filename = tmp;
	while ((tmp = strtok(NULL,"/")) != NULL)
	{
		filename = tmp ;
	}

	size_t location_size = strlen(path) - strlen(filename);
	char* location = malloc(location_size+1);
	memset(location,0,location_size+1);
	memcpy(location,path,location_size);
	//TODO: Primero buscar en cache

	log_debug(log_file,"PFS","fat32_getDirEntry() -> fat32_readDirectory(%s)",location);
	listLine_t *file_list = fat32_readDirectory(location); //Obtengo una lista de los ficheros que hay en "location"

	listNode_t *curr_file_node;
	free(location); //Libero la memoria de location

	fat32file_t *curr_file;
	dirEntry_t *direntry_found = NULL;

	while  ((curr_file_node = LIST_removeFromBegin(&file_list)) != NULL)
	{
		curr_file = (fat32file_t*) curr_file_node->data;

		if (strcmp(curr_file->long_file_name,filename) == 0) //Si existe el archivo que estoy buscando
		{
			direntry_found  = malloc(sizeof(dirEntry_t));
			memcpy(direntry_found,&(curr_file->dir_entry),sizeof(dirEntry_t)); //Copio la direntry hacia una estructura a devolver
		}

		log_debug(log_file,"PFS","fat32_getDirEntry() -> LIST_destroyNode(0x%x->data(0x%x)),FAT32FILE_T)",curr_file_node,curr_file_node->data);
		LIST_destroyNode(&curr_file_node,FAT32FILE_T);

		if (direntry_found != NULL) break;
	}

	LIST_destroyList(&file_list,FAT32FILE_T);
	free(file_list);

	return direntry_found;
}
