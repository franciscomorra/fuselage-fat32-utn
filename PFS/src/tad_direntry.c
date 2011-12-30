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

uint32_t DIRENTRY_getClusterNumber(dirEntry_t *entry)
{
	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}

queue_t DIRTABLE_interpretFromCluster(cluster_t cluster)
{
	lfnEntry_t *lfn_entry = (lfnEntry_t*) cluster.data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero

	queue_t file_list; //Creo un puntero a la cola
	QUEUE_initialize(&file_list); //Inicializo la cola

	if (*(cluster.data) == '.') //Si el primer char es '.' entonces no es el directorio raiz y debo agregar las dirEntry_t '.' y '..'
	{
		fat32file_t *pointFile = malloc(sizeof(fat32file_t));
		pointFile->long_file_name = malloc(1);
		strcpy(pointFile->long_file_name,".");
		pointFile->cluster = cluster.number;
		pointFile->offset = 0;
		pointFile->dir_entry = *((dirEntry_t*) lfn_entry++);
		QUEUE_appendNode(&file_list,pointFile);

		fat32file_t *pointpointFile = malloc(sizeof(fat32file_t));
		pointpointFile->long_file_name = malloc(2);
		strcpy(pointpointFile->long_file_name,"..");
		pointpointFile->dir_entry = *((dirEntry_t*) lfn_entry++);
		pointpointFile->cluster = cluster.number;
		pointpointFile->offset = 32;
		QUEUE_appendNode(&file_list,pointpointFile);

	}

	while (*((char*) lfn_entry) != '\x00') //Mientras el primer byte de cada 32 bytes que voy recorriendo sea distinto de 0x00 quiere decir que hay una LFN o una DIRENTRY
	{
		if (*((char*) lfn_entry) != '\xE5')
		{
			if (lfn_entry->sequence_no.number == 1 && lfn_entry->sequence_no.deleted == false)				//Si es la ultima LFN del archivo y no esta eliminada (Saltea tambien la DIRENTRY ya que las marca igual que las LFN eliminadas)
			{
				fat32file_t *new_file = malloc(sizeof(fat32file_t));
				new_file->cluster = cluster.number;
				new_file->offset = ((char*) lfn_entry) - cluster.data;
				new_file->long_file_name = malloc(14);
				LFNENTRY_getString(*lfn_entry,new_file->long_file_name);
				new_file->long_file_name[13] = '\0';
				new_file->lfn_entry = *lfn_entry;
				new_file->dir_entry = *((dirEntry_t*) ++lfn_entry);
				new_file->has_lfn = true;
				lfn_entry++;
				QUEUE_appendNode(&file_list,new_file);
			}
			else
			{
				fat32file_t *new_file = malloc(sizeof(fat32file_t));
				new_file->cluster = cluster.number;
				new_file->offset = ((char*) lfn_entry) - cluster.data;
				new_file->long_file_name = malloc(13);
				strncpy(new_file->long_file_name,((dirEntry_t*) lfn_entry)->dos_name,8);
				char *space =	strchr(new_file->long_file_name,' ');
				if (space != NULL)
				{
					strncpy(space,".",1);
					strncpy(space+1,((dirEntry_t*) lfn_entry)->dos_extension,strlen(((dirEntry_t*) lfn_entry)->dos_extension));
					strncpy(space+strlen(((dirEntry_t*) lfn_entry)->dos_extension),"\0",1);
				}
				else
				{
					strncpy(new_file->long_file_name+8,".",1);
					strncpy(new_file->long_file_name+9,((dirEntry_t*) lfn_entry)->dos_extension,3);
					strncpy(new_file->long_file_name+strlen(new_file->long_file_name),"\0",1);
				}

				new_file->dir_entry = *((dirEntry_t*) lfn_entry++);
				new_file->has_lfn = false;
				QUEUE_appendNode(&file_list,new_file);

			}
		}
		else
		{
			lfn_entry++;
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

void DIRENTRY_setDosName(dirEntry_t *entry,char* filename)
{
	memset(entry->dos_name,0,8);
	uppercase(filename);
	memcpy(entry->dos_name,filename,8);

	if (strlen(filename) > 8)
	{
		entry->dos_name[6] = '~';
		entry->dos_name[7] = '1';
	}
	return;
}

time_t DIRENTRY_convertDateTime(date_bytes date,time_bytes time)
{
	struct tm timedate;
	timedate.tm_min = time.minutes;
	timedate.tm_sec = time.seconds;
	timedate.tm_hour = time.hours;

	timedate.tm_mon = date.month-1;
	timedate.tm_mday = date.day;
	timedate.tm_year = (date.year + 1980) - 1900;

	return mktime(&timedate);
}
