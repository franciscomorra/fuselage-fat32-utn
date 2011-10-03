/*
 * tad_fat.c
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */
#include "tad_fat.h"
#include <stdlib.h>
#include <fcntl.h>



clusterNode_t* FAT_getClusterChain(fatTable_t *fat,uint32_t init_cluster)
{
	uint32_t cluster_no = init_cluster;
	clusterNode_t *new,*first;

	if (fat->table[cluster_no] == 0x0)
	{
		return 0;
	}
	else if (fat->table[cluster_no] < 0x0FFFFFF8)
	{
		clusterNode_t *last;
		last = 0x0;

		while (fat->table[cluster_no] < 0x0FFFFFF8)
		{
			new = malloc(sizeof(clusterNode_t));
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

		new = malloc(sizeof(clusterNode_t));
		new->number = cluster_no;
		new->next = 0;
		last->next=new;
	}
	else
	{
		new = malloc(sizeof(clusterNode_t));
		new->number = cluster_no;
		new->next = 0;
		first=new;
	}

	return first;
}
void FAT_cleanList(clusterNode_t* first)
{
	clusterNode_t* cur = first;
	clusterNode_t* next;

	while (cur->next != 0x0)
	{
		next=cur->next;
		free(cur);
		cur=next;
	}

	free(cur);
}

clusterNode_t* FAT_getFreeClusters(fatTable_t* FAT) {

	uint32_t cluster_no;
	clusterNode_t *new,*first, *last =0x0;



	for(cluster_no = 2;cluster_no < FAT->size;cluster_no++)
	{
		if(FAT->table[cluster_no] == 0)
		{
			new = malloc(sizeof(clusterNode_t));
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

uint32_t FAT_addCluster(clusterNode_t* first, clusterNode_t* new)
{
	clusterNode_t* aux = first;
	clusterNode_t* last = aux;

	while(aux->number < new->number)
	{
		last = aux;
		aux = aux->next;
	}

	new->next = aux;
	last->next = new;

	return 0;
}

uint32_t FAT_takeCluster(clusterNode_t* first, uint32_t clusterNumber)
{
	clusterNode_t* aux = first;
	clusterNode_t* last = aux;
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
