/*
 * tad_fat.c
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */
#include "tad_fat.h"
#include <stdlib.h>
#include <fcntl.h>



CLUSTER_NODE* FAT_getClusterChain(FAT_TABLE *fat,uint32_t init_cluster)
{
	uint32_t cluster_no = init_cluster;
	CLUSTER_NODE *new,*first;

	if (fat->table[cluster_no] == 0x0)
	{
		return 0;
	}
	else if (fat->table[cluster_no] < 0x0FFFFFF8)
	{
		CLUSTER_NODE *last;
		last = 0x0;

		while (fat->table[cluster_no] < 0x0FFFFFF8)
		{
			new = malloc(sizeof(CLUSTER_NODE));
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

		new = malloc(sizeof(CLUSTER_NODE));
		new->number = cluster_no;
		new->next = 0;
		last->next=new;
	}
	else
	{
		new = malloc(sizeof(CLUSTER_NODE));
		new->number = cluster_no;
		new->next = 0;
		first=new;
	}

	return first;
}
void FAT_cleanList(CLUSTER_NODE* first)
{
	CLUSTER_NODE* cur = first;
	CLUSTER_NODE* next;

	while (cur->next != 0x0)
	{
		next=cur->next;
		free(cur);
		cur=next;
	}

	free(cur);
}

CLUSTER_NODE* FAT_getFreeClusters(FAT_TABLE* FAT) {

	uint32_t cluster_no;
	CLUSTER_NODE *new,*first, *last =0x0;



	for(cluster_no = 2;cluster_no < FAT->size;cluster_no++)
	{
		if(FAT->table[cluster_no] == 0)
		{
			new = malloc(sizeof(CLUSTER_NODE));
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

uint32_t FAT_addCluster(CLUSTER_NODE* first, CLUSTER_NODE* new)
{
	CLUSTER_NODE* aux = first;
	CLUSTER_NODE* last = aux;

	while(aux->number < new->number)
	{
		last = aux;
		aux = aux->next;
	}

	new->next = aux;
	last->next = new;

	return 0;
}

uint32_t FAT_takeCluster(CLUSTER_NODE* first, uint32_t clusterNumber)
{
	CLUSTER_NODE* aux = first;
	CLUSTER_NODE* last = aux;
	uint32_t number;

	while(aux->number != clusterNumber)
	{
		last = aux;
		aux = aux->next;
	}

	if (aux->next == 0x0)
		exit(-1);
	else
		last->next = aux->next;

	number=(aux->number);
	free(aux);
	return(number);
}
