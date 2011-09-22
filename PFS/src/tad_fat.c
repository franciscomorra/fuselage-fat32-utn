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
	cluster_node *new,*first;

	if (fat->table[cluster_no] == 0x0)
	{
		return 0;
	}
	else if (fat->table[cluster_no] < 0x0FFFFFF8)
	{
		cluster_node *last;
		last = 0x0;

		while (fat->table[cluster_no] < 0x0FFFFFF8)
		{
			new = malloc(sizeof(cluster_node));
			new->number = cluster_no;
			new->next = 0;

			if (last == 0x0)
			{
				first=last=new;
			}
			else
			{
				last->next=new;
				last = new;
			}

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

cluster_node* FAT_getFreeClusters(FAT_struct* FAT) {

	uint32_t cluster_no;
	cluster_node *new,*first, *last =0x0;


	for(cluster_no = 2;cluster_no < FAT->size;cluster_no++)
	{
		if(FAT->table[cluster_no] == 0)
		{
			new = malloc(sizeof(cluster_node));
			new->number = cluster_no;

			if (last == 0x0)
			{
				last = first = new;
			}
			else
			{
				last->next = new;
				last = new;
			}
		}
	}

	return first;
}
