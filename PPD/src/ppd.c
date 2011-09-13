/*
 * ppd.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <math.h>

uint32_t sectors_perCluster;
uint32_t sectors_perFat;
uint32_t sectors_beforeFat;
uint32_t bytes_perSector;


int main(int argc, char *argv[])
{

	char *data = (char*)malloc(512);
	memset(data,0,512);
	leer_sector(0,data);
	//sectors_perFat = malloc(4);

	memcpy(&sectors_perFat,data+0x24,4);
	memcpy(&sectors_perCluster,data+0x0D,1);
	memcpy(&sectors_beforeFat,data+0x0E,2);
	memcpy(&bytes_perSector,data+0x0B,2);


	return 1;
}


