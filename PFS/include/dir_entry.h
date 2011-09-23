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
} __attribute__((__packed__)) directory_entry;
//___STRUCT_DIR_ENTRY

typedef struct long_filename_entry
{
	char sequence_no;
	char name_chars1[10];
	char attr;
	char reserved;
	char checksum;
	char name_chars2[12];
	char first_cluster[2];
	char name_chars3[4];

} __attribute__((__packed__)) long_filename_entry;

typedef struct file_node
{
	char *long_file_name;
	char file_name[12];
	uint32_t cluster_no;
	file_attr attr;
	directory_entry dir_entry;
	struct file_node * next;
}__attribute__((__packed__)) file_node;

typedef struct lfn_sequence_number
{
	uint32_t number :6;
	bool last 	 	:1;
	bool deleted 	:1;

}__attribute__((__packed__)) lfn_sequence_number;

//DIRENTRY_getClusterNumber: Obtiene el numero de cluster a partir de los bytes de la Directory Entry de un archivo
uint32_t DIRENTRY_getClusterNumber(directory_entry *dir_entry);

file_node* DIRENTRY_getFileList(char* cluster_data);

#endif /* DIR_ENTRY_H_ */
