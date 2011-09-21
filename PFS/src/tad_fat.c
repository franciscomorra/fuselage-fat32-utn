/*
 * tad_fat.c
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */
#include "tad_fat.h"
#include <fcntl.h>

cluster_node* FAT_getClusterChain(FAT_struct *fat,uint32_t init_cluster)
{
	uint32_t cluster_no = init_cluster;
	cluster_node *new;
	cluster_node *first;

	if (fat->table[cluster_no] == 0x0)
	{
		return 0;
	}
	else if (fat->table[cluster_no] < 0x0FFFFFF8)
	{
		cluster_node *last;
		uint32_t first_cluster = 1;
		while (fat->table[cluster_no] < 0x0FFFFFF8)
		{
			new = malloc(sizeof(cluster_node));
			new->number = cluster_no;
			new->next = 0;

			if (first_cluster)
			{
				first=new;
				first_cluster = 0;
			}
			else
			{
				last->next=new;
			}
			last = new;
			cluster_no = fat->table[cluster_no];
		}

			new = malloc(sizeof(cluster_node));
			new->number = cluster_no;
			new->next = 0;
			last->next=new;
	}
	else
	{
		new = malloc(sizeof(cluster_node));
		new->number = cluster_no;
		new->next = 0;
		first=new;
	}

	return first;
}

uint32_t FAT32_getFreeClusters(cluster_node* first) {
	uint32_t i;
	FAT_struct* FAT = malloc(sizeof(FAT_struct));
	cluster_node* new;
	cluster_node* last;

	last = 0;
	if (FAT32_readFAT(FAT) != 0)
		printf("error lectura FAT");

	for(i=3;i<(FAT->size);i++)
		if(*(FAT->table + i) == 0){
			if (last == 0){
				first->number = i;
				last = first;
			}
			else {
				new = malloc(sizeof(cluster_node));
				new->number = i;
				last->next = new;
				last = new;
				free(new);
			}
		}
	if(first == 0)
		exit(1);

	free(FAT);
	free(last);
	return 0;
}
