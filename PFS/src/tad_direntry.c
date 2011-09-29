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
#include "tad_direntry.h"
#include "utils.h"


uint32_t DIRENTRY_getClusterNumber(directory_entry *entry)
{

	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}


size_t DIRENTRY_getLongFileName(long_filename_entry lfn,char** ret_longfilename)
{
	size_t utf8_size;
	char* long_filename = malloc(26);
	*ret_longfilename = malloc(13);
	memcpy(long_filename,lfn.name_chars1,10);		// Junto las tres partes en un buffer primero
	memcpy(long_filename+10,lfn.name_chars2,12);	// y despues las voy a pasar a UTF8
	memcpy(long_filename+22,lfn.name_chars3,4);
	unicode_utf16_to_utf8_inbuffer((uint16_t*)long_filename,13,*ret_longfilename,&utf8_size);
	free(long_filename);
	return strlen(*ret_longfilename);
}


file_node* DIRENTRY_getFileList(char* cluster_data)
{
	long_filename_entry *lfn_entry = (long_filename_entry*) cluster_data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero
	char* longfilename_buf = malloc(255); //Uso este buffer para ir almacenando los LFN de un archivo
	memset(longfilename_buf,0,255); //Lo seteo a 0
	char *tmp_longfilename_part, *new_longfilename; //Puntero para UN LFN de UN archivo, y puntero para el nombre largo completo de un archivo
	size_t tmp_longfilename_part_size = 0, new_longfilename_size = 0; //Tamaño de cadena de los punteros
	file_node *last = 0x0, *first = 0x0; //Punteros a nodos de una lista que sera la que se obtenga de esta funcion

	while (*((char*) lfn_entry) != 0x00) //Mientras el primer byte de cada 32 bytes que voy recorriendo sea distinto de 0x00 quiere decir que hay una LFN o una DIRENTRY
	{
		if (lfn_entry->sequence_no.number == 1) //Si es la ultima LFN del archivo
		{
			tmp_longfilename_part_size = DIRENTRY_getLongFileName(*lfn_entry,&tmp_longfilename_part); //Obtengo la ultima parte (Viene a ser la primera del nombre , estan dispuestas al reves)
			new_longfilename_size += tmp_longfilename_part_size; //Aumento el tamaño del nombre para obtener el tamaño final
			/* Corro lo que esta almacenado en el buffer
			 * la cantidad de posiciones necesarias hacia la derecha para que entre
			 * la ultima parte del nombre
			 * */
			shiftbytes_right(longfilename_buf,255,tmp_longfilename_part_size);
			/* Copio la siguiente parte del nombre en el buffer donde
			 * quedara el nombre completo
			 */
			memcpy(longfilename_buf,tmp_longfilename_part,tmp_longfilename_part_size);
			new_longfilename = malloc(new_longfilename_size); /* Obtengo memoria para el nuevo nombre de archivo */
			memcpy(new_longfilename,longfilename_buf,new_longfilename_size); /*Copio el nombre completo que esta en el buffer a la memoria almacenada antes */
			memset(longfilename_buf,0,255); /* Seteo el buffer donde se van almacenando las partes a 0 para empezar con el siguiente archivo */
			free(tmp_longfilename_part); //Libero memoria

			/* Aca empieza a leer la DIRENTRY del archivo */
			directory_entry *direntry = (directory_entry*) ++lfn_entry;

			file_node *new_file = malloc(sizeof(file_node));
			new_file->long_file_name = malloc(new_longfilename_size);
			strcpy(new_file->long_file_name,new_longfilename);
			memcpy(&(new_file->dir_entry),direntry,sizeof(directory_entry));
			new_file->next = 0x0;

			if (first == 0x0) {
				first = last = new_file;
			} else {
				last->next = new_file;
				last = new_file;
			}

			/* Aca termina de leer la DIRENTRY del archivo */
			lfn_entry++;
			free(new_longfilename);
			new_longfilename_size = 0;
		}
		else //Si no es la ultima LFN del archivo
		{
			tmp_longfilename_part_size = DIRENTRY_getLongFileName(*lfn_entry,&tmp_longfilename_part); //Obtengo la cadena parte del nombre del LFN
			new_longfilename_size += tmp_longfilename_part_size; //Aumento el tamaño del nombre del archivo que estoy leyendo
			/* Corro lo que esta almacenado en el buffer
			 * la cantidad de posiciones necesarias hacia la derecha para que entre
			 * la siguiente parte del nombre
			 * */
			shiftbytes_right(longfilename_buf,255,tmp_longfilename_part_size);
			/* Copio la siguiente parte del nombre en el buffer donde
			 * voy uniendo las partes
			 */
			memcpy(longfilename_buf,tmp_longfilename_part,tmp_longfilename_part_size);
			free(tmp_longfilename_part); //Libero la memoria usada
			lfn_entry++; //Paso a la siguiente entrada LFN
		}
	}

	free(longfilename_buf);
	return first; //Retorno un puntero al primer file_node
}



