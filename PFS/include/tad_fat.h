/*
 * tad_fat.h
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FAT_H_
#define TAD_FAT_H_

#include "tad_line.h"
#include <stdint.h>
#include <stdlib.h>

//___STRUCT_FAT_TABLE
typedef struct {
	uint32_t *table;
	size_t size;
	uint32_t EOC;
} fatTable_t;
//___STRUCT_FAT_TABLE

//FAT_getClusterChain: Obtiene la cadena de clusters que le sigue al cluster pasado
listLine_t* FAT_getClusterChain(fatTable_t *fat,uint32_t first_cluster);

//FAT_getFreeClusters: Obtiene una lista de clusters libres
listLine_t* FAT_getFreeClusters(fatTable_t* FAT);


#endif /* TAD_FAT_H_ */
