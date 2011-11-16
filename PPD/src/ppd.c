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

#include "log.h"
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
#include "ppd_pfsList.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t HeadPosition;
uint32_t SectorJumpTime;
uint32_t bytes_perSector;
uint32_t file_descriptor;
uint32_t TracePosition;
uint32_t ReadTime;
uint32_t WriteTime;
flag_t Algorithm;
multiQueue_t* multiQueue;
sem_t mainMutex;
t_log* Log;
e_message_level LogFlag;
queue_t pfsList;

int main(int argc, char *argv[])
{
	bytes_perSector = 512;
	multiQueue = malloc(sizeof(multiQueue_t));
	QUEUE_initialize(&pfsList);
	pthread_t TAKERtid;					//thread taker
	uint32_t newFD;						//nuevo descriptor de socket de alguna nueva conexion
	uint32_t FDmax;						//mayor descriptor de socket
	uint32_t addrlen;
	uint32_t startingMode;
	uint32_t currFD;					//current fd sirve para saber que fd tuvo cambios
	uint32_t RPM;						//No es global ya que solo me interesa comunicar el SectorJumpTime
	uint32_t port;
	uint32_t diskID;
	char* IP;
	char* sockUnixPath;
	char* diskFilePath;
	char* consolePath;
	flag_t initialDirection;
	socketUnix_t consoleListen;			//estructura socket de escucha correspondiente a la consola
	socketInet_t inetListen;			//estructura socket de escucha correspondiente a la consola
	socketUnix_t newSocket;				//nueva estructura de socket que contendra datos de una nueva conexión
	struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
	fd_set masterFDs;					//conjunto total de FDs que queremos administrar
	fd_set readFDs;						//conjunto de FDs de los que deseamos recibir datos

	sem_init(&(multiQueue->queueElemSem),0,0);
	sem_init(&mainMutex,0,1);
	multiQueue->qflag = QUEUE2_ACTIVE;

/*
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
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));				//
	TrackJumpTime = atoi(CONFIG_getValue(ppd_config,"TrackJumpTime"));	//	leer archivo de configuración
	HeadPosition = atoi(CONFIG_getValue(ppd_config,"HeadPosition"));	//
	RPM = atoi(CONFIG_getValue(ppd_config,"RPM"));						//
	port = atoi(CONFIG_getValue(ppd_config,"Port"));
	diskID = atoi(CONFIG_getValue(ppd_config,"DiskID"));
	strcpy(IP,CONFIG_getValue(ppd_config,"IP"));
	if(strcmp("SSTF",CONFIG_getValue(ppd_config,"Algorithm")) == 0)		//
		Algorithm = SSTF;
	else
		Algorithm = FSCAN;
	if(strcmp("LISTEN",CONFIG_getValue(ppd_config,"StartingMode")) == 0)		//
		StartingMode = MODE_LISTEN;
	else
		StartingMode = MODE_CONNECT;
*/
	COMMON_readPPDConfig(&RPM,&port,&diskID,&startingMode,&IP,
		&sockUnixPath,&diskFilePath,&consolePath,&initialDirection);

	Log = log_create("PPD","/home/utn_so/Escritorio/logPPD",LogFlag,M_CONSOLE_DISABLE);

	multiQueue->queue1 = malloc(sizeof(queue_t));
	QUEUE_initialize(multiQueue->queue1);
	if(Algorithm == SSTF){
		multiQueue->qflag = SSTF;
		multiQueue->direction = SSTF;
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,SSTF_getNext)) 	//crea el thread correspondiente al TAKER
					perror("error creacion de thread ");
	} else {
		multiQueue->qflag = QUEUE1_ACTIVE;
		multiQueue->direction = initialDirection;											//TODO hacerlo por configuracion?
		multiQueue->queue2 = malloc(sizeof(queue_t));
		QUEUE_initialize(multiQueue->queue2);
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,FSCAN_getNext))
			perror("error creacion de thread ");
	}

	SectorJumpTime = (RPM*Sector)/60000;// RPm/60 -> RPs/1000 -> RPms*Sector = tiempo entre sectores
/*
	char* msg;
	for(i = 0; i < 6; i++){
		msg = COMM_createCharMessage(READ_SECTORS,4);				// temporal
		memcpy(msg+3,p+i,4);										// temporal
 		COMM_handleReceive(msg,1);
 		free(msg);
	}
*/
	switch(fork()){ 																	//ejecuta la consola
		case 0: 																		//si crea un nuevo proceso entra por esta rama
			execl(consolePath,NULL); 	//ejecuta la consola en el nuevo proceso
			break;
		case -1:																		//se creo mal el proceso
			perror("fork");
			break;
	}

	file_descriptor = open(diskFilePath,O_RDWR);

	consoleListen = SOCKET_unix_create(SOCK_STREAM,sockUnixPath,MODE_LISTEN);							//conecta la consola

	inetListen = SOCKET_inet_create(SOCK_STREAM,IP,port,startingMode);										//crea un descriptor de socket encargado de recibir conexiones entrantes
	if(startingMode == MODE_CONNECT)
		COMM_RaidHandshake(inetListen,diskID);

	FD_ZERO(&masterFDs);
	FD_SET(inetListen.descriptor,&masterFDs); 						//agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(consoleListen.descriptor,&masterFDs);					//agrego el descriptor de la consola al conjunto de FDs

	if(inetListen.descriptor > consoleListen.descriptor)
		FDmax = inetListen.descriptor;
	else
		FDmax = consoleListen.descriptor;

	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){															//hay datos nuevos
				if((currFD == inetListen.descriptor) && (startingMode == MODE_LISTEN)){																//nueva conexion tipo INET
					addrlen = sizeof(remoteaddr);
					if((newFD = accept(inetListen.descriptor,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
						perror("accept");
					 else {
						FD_SET(newFD,&masterFDs);
						if(newFD > FDmax)
							FDmax = newFD;
						}
					PFSLIST_addNew(&pfsList,newFD);
				}
				else if (currFD == consoleListen.descriptor){												//nueva conexion tipo UNIX
					newSocket = COMM_ConsoleAccept(consoleListen);
					FD_SET(newSocket.descriptor,&masterFDs);
					if(newSocket.descriptor > FDmax)
						FDmax = newSocket.descriptor;
					PFSLIST_addNew(&pfsList,newSocket.descriptor);
					}
				else { 																						//datos de un cliente
					uint32_t dataReceived = 0;
					pfs_node_t* in_pfs = PFSLIST_getByFd(pfsList,currFD);
					sem_wait(&in_pfs->sock_mutex);
					char* msgIn = COMM_receive(currFD,&dataReceived);
					if(dataReceived == 0){																	//si es igual a cero cierra la conexion
						close(currFD);
						FD_CLR(currFD,&masterFDs);
					} else {
						COMM_handleReceive(msgIn,currFD);
						free(msgIn);
					}
					sem_post(&in_pfs->sock_mutex);
				}
			}
		}
	}
	return 0;
}
