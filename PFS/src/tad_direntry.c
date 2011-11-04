/*
 * dir_entry.c
 *
 *  Created on: 16/09/2011
 *      Author: utn_so
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tad_bootsector.h"
#include "log.h"
#include "tad_lfnentry.h"
#include "tad_queue.h"
#include "tad_file.h"
#include "tad_direntry.h"
#include "utils.h"
#include <time.h>

extern bootSector_t boot_sector;
extern t_log *log_file;

uint32_t DIRENTRY_getClusterNumber(dirEntry_t *entry)
{
	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}


queue_t DIRENTRY_interpretTableData(clusterChain_t cluster_chain)
{
	lfnEntry_t *lfn_entry = (lfnEntry_t*) cluster_chain.data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero

	queue_t file_list; //Creo un puntero a la cola
	QUEUE_initialize(&file_list); //Inicializo la cola

	queue_t *lfn_entries;


	if (*(cluster_chain.data) == '.') //Si el primer char es '.' entonces no es el directorio raiz y debo agregar las dirEntry_t '.' y '..'
	{
		fat32file_t *pointFile = malloc(sizeof(fat32file_t));
		lfn_entries = malloc(sizeof(queue_t));
		QUEUE_initialize(lfn_entries);
		pointFile->long_file_name = malloc(1);
		strcpy(pointFile->long_file_name,".");
		pointFile->lfn_entries = *lfn_entries;

		pointFile->dir_entry = (dirEntry_t*) lfn_entry++;
		QUEUE_appendNode(&file_list,pointFile); //Lo agrego a la cola


		fat32file_t *pointpointFile = malloc(sizeof(fat32file_t));
		lfn_entries = malloc(sizeof(queue_t));
		QUEUE_initialize(lfn_entries);
		pointpointFile->long_file_name = malloc(2);
		strcpy(pointpointFile->long_file_name,"..");
		pointpointFile->lfn_entries = *lfn_entries;
		pointpointFile->dir_entry = (dirEntry_t*) lfn_entry++;
		QUEUE_appendNode(&file_list,pointpointFile);

	}

	lfn_entries = malloc(sizeof(queue_t));
	QUEUE_initialize(lfn_entries);

	while (*((char*) lfn_entry) != 0x00) //Mientras el primer byte de cada 32 bytes que voy recorriendo sea distinto de 0x00 quiere decir que hay una LFN o una DIRENTRY
	{
		//Borrado : number = 37
		if (lfn_entry->sequence_no.number == 1 && lfn_entry->sequence_no.deleted == false)				//Si es la ultima LFN del archivo y no esta eliminada (Saltea tambien la DIRENTRY ya que las marca igual que las LFN eliminadas)
		{
			QUEUE_appendNode(lfn_entries,lfn_entry++);

			fat32file_t *new_file = malloc(sizeof(fat32file_t));
			new_file->dir_entry =  (dirEntry_t*) lfn_entry++;
			new_file->lfn_entries = *lfn_entries;

			new_file->long_file_name = LFNENTRY_getLFN(new_file->lfn_entries);

			QUEUE_appendNode(&file_list,new_file); 															//Lo agrego a la cola

			lfn_entries = malloc(sizeof(queue_t));
			QUEUE_initialize(lfn_entries);


		}
		else if (lfn_entry->sequence_no.number != 1 && lfn_entry->sequence_no.deleted == false)		 //Si no es la ultima LFN del archivo
		{
			QUEUE_appendNode(lfn_entries,lfn_entry++);
		}
		else if (lfn_entry->sequence_no.deleted == true) 	//Si es una entrada eliminada la salteo
		{
			lfn_entry++; 		//Incremento 32 bytes para saltearla
		}

	}

	return file_list; //Retorno un puntero a la estructura de la cola
}

dirEntry_t DIRENTRY_create(char* filename,uint32_t cluster,uint32_t attr)
{
	dirEntry_t new_entry;
	memset(&new_entry,0,sizeof(dirEntry_t));
	memcpy(new_entry.dos_name,filename,8);
	memcpy(new_entry.low_cluster,&cluster,2);
	memcpy(new_entry.high_cluster,((char*) &cluster)+2,2);


	new_entry.create_date = DIRENTRY_getDate();
	new_entry.last_modified_date = DIRENTRY_getDate();
	new_entry.create_time = DIRENTRY_getTime();
	new_entry.last_modified_time = DIRENTRY_getTime();
	new_entry.create_time_ms = 0x00;

	if (attr == ARCHIVE_ATTR)
		new_entry.file_attribute.archive = 1;
	else if (attr == DIR_ATTR)
		new_entry.file_attribute.subdirectory = 1;

	new_entry.file_size = 0;

	return new_entry;
}

date_bytes DIRENTRY_getDate()
{
	date_bytes cur_date;
	time_t Ttime = time(NULL);
	struct tm* systime = localtime(&Ttime);
	cur_date.day =  31 & systime->tm_mday;
	cur_date.month = 15 & (systime->tm_mon+1);
	cur_date.year = 127 & (systime->tm_year - 80);
	return cur_date;
}

time_bytes DIRENTRY_getTime()
{
	time_bytes cur_time;
	time_t Ttime = time(NULL);
	struct tm* systime = localtime(&Ttime);
	cur_time.hours = 31 & systime->tm_hour;
	cur_time.minutes = 63 & systime->tm_min;
	cur_time.seconds = 31 & (systime->tm_sec/2);
	return cur_time;
}
