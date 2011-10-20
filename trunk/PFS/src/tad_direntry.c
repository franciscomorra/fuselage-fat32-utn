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
#include "tad_lfnentry.h"
#include "tad_queue.h"
#include "tad_file.h"
#include "tad_direntry.h"
#include "utils.h"


uint32_t DIRENTRY_getClusterNumber(dirEntry_t *entry)
{
	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}



queue_t* DIRENTRY_interpretDirTableData(char* table_data)
{
	lfnEntry_t *lfn_entry = (lfnEntry_t*) table_data; //Uso este puntero para recorrer el cluster_data de a 32 bytes cada vez que incremento en 1 este puntero
	char* longfilename_buf = malloc(255); //Uso este buffer para ir almacenando los LFN de un archivo
	memset(longfilename_buf, 0, 255); //Lo seteo a 0
	char *tmp_longfilename_part, *new_longfilename; //Puntero para UN LFN de UN archivo, y puntero para el nombre largo completo de un archivo
	size_t tmp_longfilename_part_size = 0, new_longfilename_size = 0; //Tamaño de cadena de los punteros
	queueNode_t *new_file_node; //Punteros a nodos de una lista que sera la que se obtenga de esta funcion
	queue_t *file_list; //Creo un puntero a la cola
	QUEUE_initialize(&file_list); //Inicializo la cola

	if (*table_data == '.') //Si el primer char es '.' entonces no es el directorio raiz y debo agregar las dirEntry_t '.' y '..'
	{
		dirEntry_t *point = (dirEntry_t*) lfn_entry; //Casteo el puntero a dirEntry_t para interpretar los bytes como una dirEntry_t
		fat32file_t *pointFile = FILE_createStruct(".",point); //Creo una struct fat32file_t que contendra el longfilename y la dirEntry_t
		new_file_node = QUEUE_createNode(pointFile); //Creo el nodo
		QUEUE_addNode(&file_list,&new_file_node); //Lo agrego a la cola
		lfn_entry++; //Incremento el puntero en 32 bytes = sizeof(lfnEntry_t)

		dirEntry_t *pointpoint = (dirEntry_t*) (lfn_entry);
		fat32file_t *pointpointFile = FILE_createStruct("..",pointpoint); //IDEM ANTERIOR
		new_file_node = QUEUE_createNode(pointpointFile);
		QUEUE_addNode(&file_list,&new_file_node);
		++lfn_entry;
	}


	while (*((char*) lfn_entry) != 0x00) //Mientras el primer byte de cada 32 bytes que voy recorriendo sea distinto de 0x00 quiere decir que hay una LFN o una DIRENTRY
	{
		//Borrado : number = 37
		if (lfn_entry->sequence_no.number == 1 && lfn_entry->sequence_no.deleted == false) //Si es la ultima LFN del archivo y no esta eliminada (Saltea tambien la DIRENTRY ya que las marca igual que las LFN eliminadas)
		{
			//CODIGO QUE PROCESA EL LONGFILENAME
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
			/* ACA TERMINA CON LA ULTIMA LFN DEL ARCHIVO Y EL NOMBRE QUEDA ALMACENADO EN new_longfilename */

			//CODIGO QUE PROCESA LA DIRENTRY
			dirEntry_t *direntry = (dirEntry_t*) ++lfn_entry; //Casteo

			fat32file_t *new_file = FILE_createStruct(new_longfilename,direntry); //Creo la estructura fat32file_t para el nuevo archivo
			queueNode_t *new_file_node = QUEUE_createNode(new_file); //Creo el nodo
			QUEUE_addNode(&file_list,&new_file_node); //Lo agrego a la cola
			/* ACA TERMINA DE LEER LA DIRENTRY DEL ARCHIVO Y CREA EL fat32file_t con el new_longfilename y la dirEntry_t */

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
			lfn_entry++; //Incremento 32 bytes para saltearla
		}

	}

	free(longfilename_buf); //Libero el buffer temporal

	return file_list; //Retorno un puntero a la estructura de la cola
}



