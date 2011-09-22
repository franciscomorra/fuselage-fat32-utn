/*
 * dir_entry.c
 *
 *  Created on: 16/09/2011
 *      Author: utn_so
 */

#include <stdint.h>
#include "dir_entry.h"

uint32_t DIRENTRY_getClusterNumber(directory_entry *entry)
{
	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}

uint32_t DIRENTRY_getLongFilename(long_filename_entry *lfn)
{

}
