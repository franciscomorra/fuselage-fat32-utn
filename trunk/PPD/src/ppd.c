/*
 * ppd.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "config_manager.h"
#include "ppd_SSTF.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
requestNode_t* first;

int main(int argc, char *argv[])
{
	first = 0;

/*	int i;
 *	uint32_t vec[7] = {512,534, 802, 498, 816, 1526, 483};
 *	uint32_t* p = malloc(7*sizeof(uint32_t));
 *
 *	memcpy(p,vec,7*4);
 * 								sirve para probar la lista del algoritmo sstf
 */
	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));

/*	for(i = 0; i < 7; i++)
 *		SSTF_addRequest(p+i);
 *								idem
 */

	return 1;
}


