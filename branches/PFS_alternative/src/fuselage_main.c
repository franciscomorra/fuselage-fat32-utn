/*
 * proceso_filesystem.c
 *
 *  Created on: 04/09/2011
 *      Author: utn_so
 *//*
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "fuselage_main.h"
#include "fuse_operations.h"
#include "pfs_fat32.h"
#include "tad_fat.h"
#include "tad_lfnentry.h"
#include "tad_queue.h"
#include "tad_file.h"
#include "tad_bootsector.h"
#include "tad_direntry.h"
#include "tad_cluster.h"
#include "tad_sector.h"
#include "file_cache.h"
#include "utils.h"
#include <time.h>
#include <sys/time.h>
#include "log.h"
#include <signal.h>

bootSector_t boot_sector;
fat_table_t fat;
t_log *log_file;

queue_t opened_files;



extern 	struct args
{
	int argc;
	char **argv;
} args;

extern char* cmd_received;
int cmd_signal();
extern pthread_mutex_t signal_lock;

void *fuselage_main (void *data)
{
	log_file = log_create("PFS","pfs.log",DEBUG,M_CONSOLE_DISABLE);
	log_debug(log_file,"PFS","Inicio PFS");

	boot_sector.bytes_perSector = 512; //Habra que hacer alguna funcion especial para leer solo el boot_sector;

	fat32_readBootSector(&boot_sector);
	FAT_read();

	signal(SIGUSR2,(__sighandler_t*) cmd_signal);

	fuse_main(args.argc,args.argv, &fuselage_oper,NULL);
	//LANZAR THREAD DE LA CONSOLA
	/*
	 *  pthread_t consoleThread;
        pthread_attr_t consoleAttr;
        pthread_attr_init(&consoleAttr);
        pthread_attr_setdetachstate(&consoleAttr, PTHREAD_CREATE_DETACHED);
        pthread_create(&consoleThread, &consoleAttr, &console, NULL);
	 *

	raise(SIGTERM);
	return 0;
}

int cmd_signal()
{
	char* token = (char*) strtok(cmd_received," ");

	if (strcmp(token,"fsinfo") == 0)
	{
			queue_t cluster_list = FAT_get_free_clusters();
			queueNode_t* cur_cluster_node = NULL;
			uint32_t count_free = 0;

			while ((cur_cluster_node = QUEUE_takeNode(&cluster_list)) != NULL)
			{
				++count_free;
				//QUEUE_freeNode(cur_cluster_node,0);
				free(cur_cluster_node->data);
				free(cur_cluster_node);
			}

			uint32_t count_notfree = fat.size - count_free;
			//QUEUE_destroy(&cluster_list,UINT32_T);

			printf("\nClusters ocupados: %d\n",count_notfree);
			printf("Clusters libres: %d\n",count_free);
			printf("Tamaño sector: %d bytes\n",boot_sector.bytes_perSector);
			printf("Tamaño cluster: %d bytes\n",(boot_sector.bytes_perSector*boot_sector.sectors_perCluster));
			printf("Tamaño FAT: %d Kb\n\n",(sizeof(uint32_t)*fat.size/1024));
			fflush(stdout);
			free(cmd_received);
	}
	else if (strcmp(token,"finfo") == 0)
	{
		token = strtok(NULL," ");


		fat32file_t *fileentry = fat32_getFileEntry(token);
		if (fileentry != NULL)
		{
			printf("#Clusters:\n");
			queue_t cluster_list = FAT_get_linked_clusters(DIRENTRY_getClusterNumber(&fileentry->dir_entry));

			uint32_t cluster_index;
			queueNode_t* cur_node;
			for (cluster_index = 0;cluster_index < 20;cluster_index++)
			{
				if ((cur_node = QUEUE_takeNode(&cluster_list)) != NULL)
				{
					uint32_t *cur_cluster = (uint32_t*) cur_node->data;
					printf("%d, ",*cur_cluster);

					free(cur_node->data);
					free(cur_node);
				}
				else
				{
					printf("\n");
					break;
				}
			}
			FILE_free(fileentry);

		}
		else
			printf("No existe el archivo\n");


	}

	pthread_mutex_unlock(&signal_lock);
	return 0;

}

*/
