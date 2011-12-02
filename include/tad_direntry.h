/*
 * dir_entry.h
 *
 *  Created on: 16/09/2011
 *      Author: utn_so
 *
 * __attribute__((__packed__)) :  Atributo para que use la menos cantidad de bytes posibles para este struct,
 * 								  sino los datos se corren porque el compilador agrega un
 * 								  espaciado de bytes para alinear los datos en memoria
 */

#ifndef DIR_ENTRY_H_
#define DIR_ENTRY_H_

#include <stdint.h>
#include <stdbool.h>
#include "tad_queue.h"
#include "tad_cluster.h"

#define ARCHIVE_ATTR 1
#define DIR_ATTR 0
//___STRUCT_FILE_ATTRIBUTE
typedef struct file_attr
{
	bool read_only		:1;
	bool hidden			:1;
	bool system			:1;
	bool volume_label	:1;
	bool subdirectory	:1;
	bool archive		:1;
	bool device 		:1;
	bool unused			:1;

}  file_attr;
//___STRUCT_FILE_ATTRIBUTE

//___STRUCT_TIME
typedef struct time_bytes
{
	uint32_t seconds	:5;
	uint32_t minutes 	:6;
	uint32_t hours 		:5;
} __attribute__((__packed__)) time_bytes;

//___STRUCT_TIME

//___STRUCT_DATE
typedef struct date_bytes
{
	uint32_t day	:5;
	uint32_t month	:4;
	uint32_t year	:7;


} __attribute__((__packed__)) date_bytes;

//___STRUCT_DATE

//___STRUCT_DIR_ENTRY
typedef struct directory_entry
{
	char dos_name[8];
	char dos_extension[3];
	file_attr file_attribute;
	char reserved;
	char create_time_ms;
	time_bytes create_time;
	date_bytes create_date;
	date_bytes last_acces_date;
	char high_cluster[2]; //Orden correcto: high_cluster[1] high_cluster[1] L L
	time_bytes last_modified_time;
	date_bytes last_modified_date;
	char low_cluster[2]; //Orden correcto: H H low_cluster[1] low_cluster[1]
	uint32_t file_size	:32;
} __attribute__((__packed__)) dirEntry_t;
//___STRUCT_DIR_ENTRY



//DIRENTRY_getClusterNumber: Obtiene el numero de cluster a partir de los bytes de la Directory Entry de un archivo
uint32_t DIRENTRY_getClusterNumber(dirEntry_t *dir_entry);

//DIRENTRY_interpretTableData: Interpreta una tabla de directorio y devuelve una lista de todos los archivos/directorios que aparecen en ella
queue_t DIRENTRY_interpretTableData(cluster_set_t cluster_chain);

queue_t DIRTABLE_interpretFromCluster(cluster_t cluster);

dirEntry_t DIRENTRY_create(char* filename,uint32_t cluster,uint32_t attr);

date_bytes DIRENTRY_getDate();

time_bytes DIRENTRY_getTime();

void DIRENTRY_setDosName(dirEntry_t *entry,char* filename);

#endif /* DIR_ENTRY_H_ */
