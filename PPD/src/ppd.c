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
#include <unistd.h>			//entre otras cosas sleep()
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <poll.h> 		// poll
#include <sys/time.h>		// select
#include <unistd.h>			// select
#include <netinet/in.h>
#include "nipc.h"
#include "config_manager.h"
#include "ppd_queue.h"
#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_comm.h"
#include "ppd_taker.h"
#include "ppd_cHandler.h"
#include "ppd_translate.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t headPosition;
uint32_t SectorJumpTime;
uint32_t bytes_perSector;
uint32_t file_descriptor;
queue_t* queue;
sem_t queueElemSem;
sem_t mainMutex;

//struct pollfd pollFds[2];

int main(int argc, char *argv[])
{
	file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
	bytes_perSector = 512;
	queue = malloc(sizeof(queue_t));
	pthread_t TAKERtid;					//thread taker
	fd_set masterFDs;					//conjunto total de FDs que queremos administrar
	fd_set readFDs;						//conjunto de FDs de los que deseamos recibir datos
	uint32_t newFD;						//nuevo descriptor de socket de alguna nueva conexion
	uint32_t FDmax;						//mayor descriptor de socket
	uint32_t consoleFD;					//FD correspondiente a la consola
	uint32_t listenFD;					//FD encargado de escuchar nuevas conexiones
	struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
	uint32_t addrlen;
	uint32_t currFD;					//current fd sirve para saber que fd tuvo cambios
	uint32_t RPM;						//No es global ya que solo me interesa comunicar el SectorJumpTime

	sem_init(&queueElemSem,0,0);
	sem_init(&mainMutex,0,1);

	int i;													// temporal
 	uint32_t vec[7] = {512,534, 802, 498, 816, 1526, 483};	// temporal
 	uint32_t* p = malloc(7*sizeof(uint32_t));				// temporal
 	memcpy(p,vec,7*4);										// temporal

	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));			//
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));					//
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));				//	leer archivo de configuración
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));	//
	headPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));	//
	RPM = atoi(CONFIG_getValue(ppd_config,"RPM"));						//

	SectorJumpTime = RPM / 60000 * Sector;// RPm/60 -> RPs/1000 -> RPms*Sector = tiempo entre sectores

	char* msg;
	for(i = 0; i < 7; i++){
		msg = COMM_createCharMessage(READ_SECTORS,4);				// temporal
		memcpy(msg+3,p+i,4);										// temporal
 		ppd_receive(msg,1);
	}
	if(pthread_create(&TAKERtid,NULL,(void*)&TAKER_main,NULL)) //crea el thread correspondiente al TAKER
			perror("error creacion de thread ");

	switch(fork()){ 																	//ejecuta la consola
		case 0: 																		//si crea un nuevo proceso entra por esta rama
			execl("/home/utn_so/Desktop/trabajos/PPD_Console/Debug/PPD_Console",NULL); 	//ejecuta la consola en el nuevo proceso
			break;
		case -1:																		//se creo mal el proceso
			perror("fork");
			break;
	}

	CHANDLER_connect(&consoleFD);		//conecta la consola
	COMM_connect(&listenFD);			//crea un descriptor de socket encargado de recibir conexiones entrantes

	FD_ZERO(&masterFDs);
	FD_SET(listenFD,&masterFDs); 		//agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(consoleFD,&masterFDs);		//agrego el descriptor de la consola al conjunto de FDs

	if(listenFD > consoleFD)
		FDmax = listenFD;
	else
		FDmax = consoleFD;

	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){	//hay datos nuevos
				if( currFD == listenFD){	//nueva conexion
					addrlen = sizeof(remoteaddr);
					if((newFD = accept(listenFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
						perror("accept");
					 else {
						FD_SET(newFD,&masterFDs);
						if(newFD > FDmax)
							FDmax = newFD;
					}
				} else { //datos de un cliente
					uint32_t recvReturn = 0;
					char* msgIn = malloc(bytes_perSector + 7);
					//char msgIn[bytes_perSector + 7];
					if((recvReturn = recv(currFD,msgIn,519,0)) == 0)
					{
						close(currFD);
						FD_CLR(currFD,&masterFDs);
					} else
						ppd_receive(msgIn,currFD);
					free(msgIn);
				}
			}
		}
	}

/*
 CHANDLER_connect(&pollFds[0].fd); //posicion 0 del pollFds siempre perteneciente a la consola del ppd


	while(1){
		char* msgIn = malloc(bytes_perSector + 3);

		pollFds[0].events = POLLIN;	 //seteo para que me avise cuando puedo recibir datos
		pollReturn = poll(pollFds,2,-1);
		if(pollReturn == -1){
			perror("poll");
		} else if(pollReturn == 0) {
			printf("Timeout occurred! No data after 3.5 seconds \n");
		} else { //se fija por eventos de la consola
			if(pollFds[0].revents & POLLIN){
				recv(pollFds[0].fd,msgIn,sizeof(msgIn),NULL);
				ppd_recei65536ve(msgIn,0);
			}
			free(msgIn);
		}
	}
*/
return 0;
}

void TAKER_main() {
	while(1){
		sem_wait(&queueElemSem);
		sem_wait(&mainMutex);

		requestNode_t* request = SSTF_takeRequest(queue);

		TAKER_handleRequest(queue,request);
		char* msg = TRANSLATE_fromRequestToChar(request);
		ppd_send(msg,request->sender);

		uint32_t a;
		memcpy(&a,msg+3,4);
		printf("%d\n",a);									//temporal, muestra los sectores atendidos
		fflush(0);											//hace que no se acumulen datos y los largue de a tandas

		free(msg);
		free(request);
		sem_post(&mainMutex);
	}
}
