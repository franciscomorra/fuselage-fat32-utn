#include "ppd_io.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>


int main () {
	cluster_node* first;
	uint32_t sectors_per_fat = 1024;
	FAT_struct* fat=0x0;

	printf("iniciando prueba, \n");


	if (FAT32_readFAT(fat,sectors_per_fat) != 0)
		printf("error readFat");

	first = FAT_getFreeClusters(fat);
	if(first == 0x0)
		printf("no hay clusters libres");
	printf("%d ",first->number);

	free(first);
	return 0;
}

