/*
 * ppd.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

int32_t sectors_perCluster;
int32_t sectors_perFat;
int32_t sectors_beforeFat;
int32_t bytes_perSector;


int main(int argc, char *argv[])
{
	int32_t fd = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);

	char *data = (char*)malloc(512);
	memset(data,0,512);
	leer_sector(fd,0,data);
	//sectors_perFat = malloc(4);

	memcpy(&sectors_perFat,data+0x24,4);
	memcpy(&sectors_perCluster,data+0x0D,1);
	memcpy(&sectors_beforeFat,data+0x0E,2);
	memcpy(&bytes_perSector,data+0x0B,2);


	return 1;
}


