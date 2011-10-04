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
#include "tad_filenode.h"
#include "tad_lfnentry.h"
#include "log.h"

extern bootSector_t boot_sector;
extern t_log *log;

uint32_t fat32_readFAT(fatTable_t *fat)
{
	log_debug(log,"PFS","Leyendo FAT Table");
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
	log_debug(log,"PFS","FAT Table OK");
	fat->EOC = *(fat->table + 1);
	return 0;
}


uint32_t fat32_readBootSector(bootSector_t *bs)
{
	log_debug(log,"PFS","Leyendo Boot Sector");
	uint32_t sectors[1] = {0} ;
	char *bootsector_data = PFS_requestSectorsOperation(READ_SECTORS,sectors,1);
	assert(*bootsector_data != 0x00);
	log_debug(log,"PFS","Boot Sector OK");
	memcpy(bs,bootsector_data,512);
	free(bootsector_data);
	return 0;

}

uint32_t fat32_getClusterData(uint32_t cluster_no,char** buf)
{
	uint32_t *sectors = (uint32_t*) cluster_to_sectors(cluster_no);
	*buf = PFS_requestSectorsOperation(READ_SECTORS,sectors,8);
	log_debug(log,"PFS","Leyendo Cluster %d (Sectores: %d %d %d %d %d %d %d %d)",cluster_no,
	sectors[0],sectors[1],sectors[2],sectors[3],sectors[4],sectors[5],sectors[6],sectors[7]);
	free(sectors);
	return 0;
}

fileNode_t* fat32_readDirectory(const char* path)
{
	char *token,*buf;
	bool dir_exists = false;
	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);

	fat32_getClusterData(2,&buf);
	fileNode_t* list = fat32_getFileList(buf);
	free(buf);
	if (strcmp(path,"/") == 0) return list;


	token = strtok(string,"/");

	fileNode_t* curr;
	do
	{
		dir_exists = false;
		while ((curr = FILENODE_takeNode(&list)) != NULL)
		{
			if (strcmp(curr->long_file_name,token) == 0 && (curr->dir_entry.file_attribute.subdirectory) == true)
			{
				dir_exists=true;
				FILENODE_cleanList(list);
				fat32_getClusterData(DIRENTRY_getClusterNumber(&(curr->dir_entry)),&buf);
				list = fat32_getFileList(buf);
				free(buf);
				break;
			}
		}

		if (dir_exists == false)
		{
			FILENODE_cleanList(list);
			return NULL;
		}

	}
	while((token = strtok( NULL, "/" )) != NULL && dir_exists == true);

	return list;
}

fileNode_t* fat32_getFileList(char* cluster_data) {
	lfnEntry_t *lfn_entry = (lfnEntry_t*) cluster_data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero
	char* longfilename_buf = malloc(255); //Uso este buffer para ir almacenando los LFN de un archivo
	memset(longfilename_buf, 0, 255); //Lo seteo a 0
	char *tmp_longfilename_part, *new_longfilename; //Puntero para UN LFN de UN archivo, y puntero para el nombre largo completo de un archivo
	size_t tmp_longfilename_part_size = 0, new_longfilename_size = 0; //Tamaño de cadena de los punteros
	fileNode_t *first = NULL; //Punteros a nodos de una lista que sera la que se obtenga de esta funcion

	if (*cluster_data == '.')
	{
		dirEntry_t *point = (dirEntry_t*) lfn_entry;
		fileNode_t *first_node =  FILENODE_createNode(".",point);
		FILENODE_addNode(&first,&first_node);
		lfn_entry++;
		dirEntry_t *pointpoint = (dirEntry_t*) (lfn_entry);
		fileNode_t *second_node = FILENODE_createNode("..",pointpoint);
		FILENODE_addNode(&(first->next),&second_node);
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

			/* Aca empieza a leer la DIRENTRY del archivo (se podria cambiar por una funcion que cree el nodo) */
			dirEntry_t *direntry = (dirEntry_t*) ++lfn_entry;

			fileNode_t *new_node = FILENODE_createNode(new_longfilename,direntry);
			FILENODE_addNode(&first,&new_node);

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
	return first; //Retorno un puntero al primer FILE_NODE
}

dirEntry_t* fat32_getAttr(char* path)
{
	size_t len = strlen(path);
	char string[len];
	strcpy(string,path);
	char *tmp = strtok(string,"/");
	char *filename = tmp;
	while ((tmp = strtok(NULL,"/")) != NULL)
	{
		filename = tmp;
	}

	size_t location_size = strlen(path) - strlen(filename);
	char* location = malloc(location_size+1);
	memset(location,0,location_size+1);
	memcpy(location,path,location_size);

	fileNode_t *list = fat32_readDirectory(location); //Obtengo una lista de los ficheros que hay en "location"
	free(location); //Libero la memoria de location

	fileNode_t *file_found;
	dirEntry_t *found;
	if ((file_found = FILENODE_searchNode(filename,list)) != NULL) //Si existe el archivo que estoy buscando
	{
		found  = malloc(sizeof(dirEntry_t));
		memcpy(found,&(file_found->dir_entry),sizeof(dirEntry_t)); //Copio la direntry hacia una estructura a devolver
	}
	else //Si no existe
	{
		found = NULL;
	}

	FILENODE_cleanList(list);
	return found;
}
