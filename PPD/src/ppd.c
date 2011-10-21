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
#include "ppd_taker.h"


uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t headPosition;
uint32_t bytes_perSector;
requestNode_t* first;
uint32_t file_descriptor;
queue_t* queue;
sem_t queueElemSem;
sem_t mainMutex;

int main(int argc, char *argv[])
{
	first = 0;
	file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
	bytes_perSector = 512;
	queue = malloc(sizeof(queue_t));
	pthread_t TAKERtid;

	sem_init(&queueElemSem,0,0);
	sem_init(&mainMutex,0,1);



	int i;													// temporal
 	uint32_t vec[7] = {512,534, 802, 498, 816, 1526, 483};	// temporal
 	uint32_t* p = malloc(7*sizeof(uint32_t));				// temporal
 	memcpy(p,vec,7*4);										// temporal

	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));
	headPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));

/*	switch(fork()){
		case 0:
			execl("/home/utn_so/Desktop/trabajos/PPD_Console/Debug/PPD_Console",NULL);
			break;
		case -1:
			perror(fork);
			break;
	}
*/

	for(i = 0; i < 7; i++){												 // temporal
		nipcMsg_t msgIn = NIPC_createMsg(READ_SECTORS,4,p+i);			 // temporal
 		ppd_receive(msgIn);												 // temporal
	}
	if(pthread_create(&TAKERtid,NULL,(void*)&TAKER_main,NULL))
			perror("error creacion de thread ");

	while(1);

	return 1;
}

void TAKER_main() {
	//lo mismo que el SSTF_main tuve que definirlo aca por un tema de threads
	while(1){
		sem_wait(&queueElemSem);
		sem_wait(&mainMutex);

		requestNode_t* request = SSTF_takeRequest(queue);

		//ppd_send(TAKER_handleRequest(request));

		nipcMsg_t nodo;									//temporal
		nodo = TAKER_handleRequest(request);			//temporal
		uint32_t a;										//temporal
		memcpy(&a,nodo.payload,4);						//temporal
		printf("%d\n",a);								//temporal

		free(request);
		sem_post(&mainMutex);
	}
}
