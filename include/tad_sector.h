/*
 * tad_sector.h
 *
 *  Created on: 20/10/2011
 *      Author: utn_so
 */

#ifndef TAD_SECTOR_H_
#define TAD_SECTOR_H_

#include <stdbool.h>
#include "tad_queue.h"

typedef struct sector_t
{
	uint32_t number;
	size_t size;
	char* data;
	bool modified ;
} sector_t;


#endif /* TAD_SECTOR_H_ */
