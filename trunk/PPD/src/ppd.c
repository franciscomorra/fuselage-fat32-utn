/*
 * ppd.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include "nipc.h"
#include "config_manager.h"
#include "ppd_queue.h"
#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_comm.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t headPosition;
uint32_t bytes_perSector;
requestNode_t* first;
uint32_t file_descriptor;
queue_t* queue;

int main(int argc, char *argv[])
{
	first = 0;
	file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
	bytes_perSector = 512;
	queue = malloc(sizeof(queue_t));
	queue->head = 0;
	queue->tail = 0;
	sem_init(&queue->sem,0,0);
	pthread_t SSTFtid;


	int i;
 	uint32_t vec[7] = {512,534, 802, 498, 816, 1526, 483};
 	uint32_t* p = malloc(7*sizeof(uint32_t));

 	memcpy(p,vec,7*4);

	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));
	headPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));

	if(pthread_create(&SSTFtid,NULL,(void*)&SSTF_main,NULL))
		perror("error creacion de thread SSTF");


/*	switch(fork()){
		case 0:
			execl("/home/utn_so/Desktop/trabajos/PPD_Console/Debug/PPD_Console",NULL);
			break;
		case -1:
			perror(fork);
			break;
	}

*/

	for(i = 0; i < 7; i++){
		nipcMsg_t msgIn = NIPC_createMsg(READ_SECTORS,4,p+i);
 		ppd_receive(msgIn);

	}
/*	for(i = 0; i < 7; i++){
		requestNode_t* new;

		new = QUEUE_take(queue);
		SSTF_addRequest(new);
	}
*/

	return 1;
}

uint32_t SSTF_main(void){
	//me tira error de kernel al crear el thread si defino esta funcion dentro del PPD_SSTF.c
	//lo defini en el ppd_SSTF.h
	while(1){
		requestNode_t* new;

		new = QUEUE_take(queue);
		SSTF_addRequest(new);
	}
	return 0;
}
