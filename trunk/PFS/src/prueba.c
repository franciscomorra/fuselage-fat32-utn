#include "ppd_io.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>


int main () {
	cluster_node* firstFreeCluster = malloc(sizeof(cluster_node));
	uint32_t i;

	printf("iniciando prueba\n");
	if(FAT32_getFreeClusters(firstFreeCluster) == 1)
		printf("no hay clusters libres");

	printf("numero de cluster libre: ");
	for(i=0;i<= 10;i++)
		printf("%d ",firstFreeCluster[i].number);

	free(firstFreeCluster);
	return 0;
}

