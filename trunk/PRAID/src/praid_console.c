/*
 * praid_console.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include "praid_console.h"

uint32_t print_Console (char *message){

	printf("%s \n",message);
	return 0;
}
