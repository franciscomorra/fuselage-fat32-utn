/*
 * tad_fat.c
 *
 *  Created on: 19/09/2011
 *      Author: utn_so
 */
#include "tad_fat.h"
#include <stdlib.h>
#include <fcntl.h>
#include "log.h"

extern t_log* log_file;

queue_t FAT_getClusterChain(fatTable_t *fat,uint32_t init_cluster)
{


	uint32_t  *cluster_number, cluster_no = init_cluster;
	queueNode_t *new_cluster_node;
	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);

	if (fat->table[cluster_no] == 0x00)
	{
		return cluster_list;
	}
	else if (fat->table[cluster_no] != fat->EOC)
	{
		while (fat->table[cluster_no] != fat->EOC)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
			new_cluster_node = QUEUE_createNode(cluster_number);
			QUEUE_appendNode(&cluster_list,new_cluster_node);

			cluster_no = fat->table[cluster_no];
		}

		cluster_number = malloc(sizeof(uint32_t));
		*cluster_number = cluster_no;
		log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
		new_cluster_node = QUEUE_createNode(cluster_number);

		QUEUE_appendNode(&cluster_list,new_cluster_node);
	}
	else
	{
		cluster_number = malloc(sizeof(uint32_t));
		*cluster_number = cluster_no;
		log_debug(log_file,"PFS","FAT_getClusterChain() -> LIST_addNode(0x%x,0x%x)",cluster_list,cluster_number);
		new_cluster_node = QUEUE_createNode(cluster_number);

		QUEUE_appendNode(&cluster_list,new_cluster_node);
	}

	return cluster_list;
}

queue_t FAT_getFreeClusters(fatTable_t* FAT) {
	uint32_t cluster_no, *cluster_number;
	queueNode_t *new_cluster_node;
	queue_t cluster_list;
	QUEUE_initialize(&cluster_list);

	for(cluster_no = 2;cluster_no < FAT->size;cluster_no++)
	{
		if(FAT->table[cluster_no] == 0)
		{
			cluster_number = malloc(sizeof(uint32_t));
			*cluster_number = cluster_no;
			new_cluster_node = QUEUE_createNode(cluster_number);
			QUEUE_appendNode(&cluster_list,new_cluster_node);
		}
	}


	return cluster_list;
}


