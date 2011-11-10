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
#include <sys/time.h>		// select
#include <unistd.h>			// select
#include <netinet/in.h>

#include "nipc.h"
#include "config_manager.h"
#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_comm.h"
#include "ppd_taker.h"
#include "ppd_translate.h"
#include "tad_queue.h"
#include "ppd_qManager.h"
#include "comm.h"
#include "tad_sockets.h"
#include "ppd_FSCAN.h"

#define SOCK_PATH "/home/utn_so/CONSOLE_socket"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t headPosition;
uint32_t SectorJumpTime;
uint32_t bytes_perSector;
uint32_t file_descriptor;
uint32_t NextDelay;
flag_t Algorithm;
multiQueue_t* multiQueue;
sem_t mainMutex;

int main(int argc, char *argv[])
{
	file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
	bytes_perSector = 512;
	multiQueue = malloc(sizeof(multiQueue_t));
	pthread_t TAKERtid;					//thread taker
	uint32_t newFD;						//nuevo descriptor de socket de alguna nueva conexion
	uint32_t FDmax;						//mayor descriptor de socket
	uint32_t listenFD;					//FD encargado de escuchar nuevas conexiones
	uint32_t addrlen;
	uint32_t currFD;					//current fd sirve para saber que fd tuvo cambios
	uint32_t RPM;						//No es global ya que solo me interesa comunicar el SectorJumpTime
	socketUnix_t consoleListen;			//estructura socket de escucha correspondiente a la consola
	socketUnix_t newSocket;				//nueva estructura de socket que contendra datos de una nueva conexión
	struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
	fd_set masterFDs;					//conjunto total de FDs que queremos administrar
	fd_set readFDs;						//conjunto de FDs de los que deseamos recibir datos
	sem_init(&(multiQueue->queueElemSem),0,0);
	sem_init(&mainMutex,0,1);
	multiQueue->qflag = QUEUE2_ACTIVE;

	int i;														// temporal
 	//uint32_t vec[7] = {512,534, 802, 498, 816, 1526, 483};	// temporal
	uint32_t vec[6] = {176,199,191,200,300,50};
	//uint32_t vec[1] = {0};
 	uint32_t* p = malloc(6*sizeof(uint32_t));					// temporal
 	memcpy(p,vec,6*4);											// temporal

	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder   = atoi(CONFIG_getValue(ppd_config,"Cylinder"));			//
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));					//
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));				//	leer archivo de configuración
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));	//
	headPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));	//
	RPM = atoi(CONFIG_getValue(ppd_config,"RPM"));
	if(strcmp("SSTF",CONFIG_getValue(ppd_config,"Algorithm")) == 0)
		Algorithm = SSTF;
	else
		Algorithm = FSCAN;

	multiQueue->queue1 = malloc(sizeof(queue_t));
	QUEUE_initialize(multiQueue->queue1);
	if(Algorithm == SSTF){
		multiQueue->qflag = SSTF;
		multiQueue->direction = SSTF;
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,SSTF_getNext)) 				//crea el thread correspondiente al TAKER
					perror("error creacion de thread ");
	} else {
		multiQueue->qflag = QUEUE1_ACTIVE;
		multiQueue->direction = UP;						//TODO hacerlo por configuracion?
		multiQueue->queue2 = malloc(sizeof(queue_t));
		QUEUE_initialize(multiQueue->queue2);
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,FSCAN_getNext))
			perror("error creacion de thread ");
	}

	SectorJumpTime = (RPM*Sector)/60000;// RPm/60 -> RPs/1000 -> RPms*Sector = tiempo entre sectores

	char* msg;
	for(i = 0; i < 6; i++){
		msg = COMM_createCharMessage(READ_SECTORS,4);				// temporal
		memcpy(msg+3,p+i,4);										// temporal
 		COMM_handleReceive(msg,1);
 		free(msg);
	}

	switch(fork()){ 																	//ejecuta la consola
		case 0: 																		//si crea un nuevo proceso entra por esta rama
			execl("/home/utn_so/Desarrollo/Workspace/PPD_Console/Debug/PPD_Console",NULL); 	//ejecuta la consola en el nuevo proceso
			break;
		case -1:																		//se creo mal el proceso
			perror("fork");
			break;
	}

	consoleListen = SOCKET_unix_create(SOCK_STREAM,SOCK_PATH,MODE_LISTEN);					//conecta la consola

	COMM_connect(&listenFD);																//crea un descriptor de socket encargado de recibir conexiones entrantes

	FD_ZERO(&masterFDs);
	FD_SET(listenFD,&masterFDs); 						//agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(consoleListen.descriptor,&masterFDs);		//agrego el descriptor de la consola al conjunto de FDs

	if(listenFD > consoleListen.descriptor)
		FDmax = listenFD;
	else
		FDmax = consoleListen.descriptor;

	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){															//hay datos nuevos
				if(currFD == listenFD){																//nueva conexion tipo INET
					addrlen = sizeof(remoteaddr);
					if((newFD = accept(listenFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
						perror("accept");
					 else {
						FD_SET(newFD,&masterFDs);
						if(newFD > FDmax)
							FDmax = newFD;
						}
				}
				else if (currFD == consoleListen.descriptor){												//nueva conexion tipo UNIX
					newSocket = COMM_ConsoleAccept(consoleListen);
					FD_SET(newSocket.descriptor,&masterFDs);
					if(newSocket.descriptor > FDmax)
						FDmax = newSocket.descriptor;
					}
				else { 																						//datos de un cliente
					uint32_t dataRecieved = 0;
					char* msgIn = COMM_recieve(currFD,&dataRecieved);
					if(dataRecieved == 0){																	//si es igual a cero cierra la conexion
						close(currFD);
						FD_CLR(currFD,&masterFDs);
					} else {

						COMM_handleReceive(msgIn,currFD);
						free(msgIn);
					}
				}
			}
		}
	}
	return 0;
}

void TAKER_main(uint32_t(*getNext)(queue_t*,queueNode_t**)){

	while(1){
		sem_wait(&multiQueue->queueElemSem);
		queue_t* queue = QMANAGER_selectActiveQueue(multiQueue);
		request_t* request;
		queueNode_t* prevCandidate = NULL;

		sem_wait(&mainMutex);
		uint32_t delay = getNext(queue,&prevCandidate);
		request = TAKER_takeRequest(queue,prevCandidate,&delay);
		sem_post(&mainMutex);

		TAKER_handleRequest(queue,request,delay,getNext);

		char* msg = TRANSLATE_fromRequestToChar(request);
		COMM_send(msg,request->sender);

		uint32_t a;
		memcpy(&a,msg+3,4);
		printf("%d\n",a);									//temporal, muestra los sectores atendidos
		fflush(0);											//hace que no se acumulen datos y los largue de a tandas

		free(msg);
		free(request->payload);
		free(request->CHS);
		free(request);

	}
}
