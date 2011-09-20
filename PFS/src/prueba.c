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

	printf("iniciando prueba\n");
	FAT32_getFreeClusters(firstFreeCluster);
	printf("numero de cluster libre %d",firstFreeCluster->number);

	free(firstFreeCluster);
	return 0;
}

