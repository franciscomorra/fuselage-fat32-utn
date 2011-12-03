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
#include <assert.h>

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
#include "ppd_io.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;
uint32_t TrackJumpTime;
uint32_t HeadPosition;
uint32_t SectorJumpTime;
uint32_t bytes_perSector;
int32_t file_descriptor;
uint32_t TracePosition;
uint32_t ReadTime;
uint32_t WriteTime;
flag_t Algorithm;
multiQueue_t* multiQueue;
sem_t queueMutex;
//sem_t mainMutex;
sem_t queueAvailableMutex;
t_log* Log;
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
	uint32_t port;
	uint32_t diskID;
	uint32_t exit = 0;
	char* IP;
	char* sockUnixPath;
	char* diskFilePath;
	char* consolePath;
	char* logPath;
	flag_t initialDirection;
	e_message_level logFlag;
	socketUnix_t consoleListen;			//estructura socket de escucha correspondiente a la consola
	socketInet_t inetListen;			//estructura socket de escucha correspondiente a la consola
	socketUnix_t consoleFD;				//nueva estructura de socket que contendra datos de una nueva conexión
	struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
	fd_set masterFDs;					//conjunto total de FDs que queremos administrar
	fd_set readFDs;						//conjunto de FDs de los que deseamos recibir datos

	sem_init(&(multiQueue->queueElemSem),0,0);
//	sem_init(&mainMutex,0,1);
	sem_init(&queueMutex,0,1);
	sem_init(&queueAvailableMutex,0,5000);
	multiQueue->qflag = QUEUE2_ACTIVE;

	COMMON_readPPDConfig(&port,&diskID,&startingMode,&IP,
		&sockUnixPath,&diskFilePath,&consolePath,&logPath,&initialDirection,&logFlag);

	const char arg0[10];
	const char arg1[10];
	sprintf(arg0,"%d",Sector);
	sprintf(arg1,"%d",Head);

	switch(fork()){ 																	//ejecuta la consola
		case 0: {																		//si crea un nuevo proceso entra por esta rama
			execl(consolePath,consolePath,arg0,arg1,sockUnixPath,NULL); 			//ejecuta la consola en el nuevo proceso
			break;
		}
		case -1:
			log_error(Log,"Principal","Error en la creación del fork()");
			break;
		}

	if(logFlag == OFF){
		Log = malloc(sizeof(t_log));
		pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
		Log->mutex = mutex;
		Log->file =  NULL;
	}
	else
		Log = log_create("PPD",logPath,logFlag,M_CONSOLE_DISABLE);

	multiQueue->queue1 = malloc(sizeof(queue_t));
	QUEUE_initialize(multiQueue->queue1);
	if(Algorithm == SSTF){
		multiQueue->qflag = SSTF;
		multiQueue->direction = SSTF;
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,SSTF_getNext)) 	//crea el thread correspondiente al TAKER
			log_error(Log,"Principal","Error en la creación thread con algoritmo SSTF");
	} else {
		multiQueue->qflag = QUEUE1_ACTIVE;
		multiQueue->direction = initialDirection;
		multiQueue->queue2 = malloc(sizeof(queue_t));
		QUEUE_initialize(multiQueue->queue2);
		if(pthread_create(&TAKERtid,NULL,(void*)TAKER_main,FSCAN_getNext))
			log_error(Log,"Principal","Error en la creación thread con algoritmo FSCAN");
	}

	uint32_t file_descriptor = IO_openDisk(diskFilePath);

	consoleListen = SOCKET_unix_create(SOCK_STREAM,sockUnixPath,MODE_LISTEN);							//conecta la consola

	inetListen = SOCKET_inet_create(SOCK_STREAM,IP,port,startingMode);										//crea un descriptor de socket encargado de recibir conexiones entrantes

	if(startingMode == MODE_CONNECT){
		char* payload = malloc(8);
		*((uint32_t*) (payload)) = diskID;
		*((uint32_t*) (payload+4)) =  Cylinder * Sector * Head;
		COMM_sendHandshake(inetListen.descriptor,payload,8);
		free(payload);
		uint32_t recv = 0;
		char* hndshk = COMM_receiveHandshake(inetListen.descriptor,&recv);
		free(hndshk);
		PFSLIST_addNew(&pfsList,inetListen.descriptor);
	}
	FD_ZERO(&masterFDs);
	FD_SET(inetListen.descriptor,&masterFDs); 						//agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(consoleListen.descriptor,&masterFDs);					//agrego el descriptor de la consola al conjunto de FDs

	if(inetListen.descriptor > consoleListen.descriptor)
		FDmax = inetListen.descriptor;
	else
		FDmax = consoleListen.descriptor;
/* PRUEBA
	char* msgPrueba = malloc(20+3);
	char* payload = malloc(20);
	uint32_t i = 0;
	memcpy(payload,&i,sizeof(uint32_t));
	memcpy(payload+4,&i,sizeof(uint32_t));
	memcpy(payload+8,"JAMES BOND",sizeof("JAMES BOND"));
	NIPC_createCharMsg(msgPrueba,WRITE_SECTORS,20,payload);
	COMM_handleReceive(msgPrueba,0);
*/
	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		//sem_wait(&mainMutex);
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
						char* handshake = malloc(3);
						int32_t result = SOCKET_recvAll(newFD,handshake,3,NULL);
						if (handshake[0] == HANDSHAKE && *((uint16_t*) (handshake+1)) == 0){
							SOCKET_sendAll(newFD,handshake,3,0);
							PFSLIST_addNew(&pfsList,newFD);
						}
					}
				}
				else if ((currFD == consoleListen.descriptor)&&(consoleListen.status != SOCK_DISCONNECTED)){												//nueva conexion tipo UNIX
					consoleFD = COMM_ConsoleAccept(consoleListen);
					FD_SET(consoleFD.descriptor,&masterFDs);
					if(consoleFD.descriptor > FDmax)
						FDmax = consoleFD.descriptor;
					PFSLIST_addNew(&pfsList,consoleFD.descriptor);
					close(consoleListen.descriptor);
					FD_CLR(currFD,&masterFDs);
					consoleListen.status = SOCK_DISCONNECTED;
				}
				else
				{ 																						//datos de un cliente
					uint32_t dataRecieved = 0;
					uint32_t msg_len = 0;
					pfs_node_t *in_pfs = PFSLIST_getByFd(pfsList,currFD);

					pthread_mutex_lock(&in_pfs->sock_mutex);

					//char* msg_buf = COMM_receiveAll(currFD,&dataRecieved,&msg_len);
					char* msg_buf = malloc(3);													//version santi
					int32_t result = SOCKET_recvAll(currFD,msg_buf,3,NULL);
//					assert(*((uint16_t*)(msg_buf+1)) == 8 || *((uint16_t*)(msg_buf+1)) == 520);
					msg_buf = realloc(msg_buf,*((uint16_t*)(msg_buf+1)) + 3);
					if(*((uint16_t*)(msg_buf+1)) != 0)
						result = SOCKET_recvAll(currFD,msg_buf+3,*((uint16_t*)(msg_buf+1)),NULL);
//					assert(result == 520 || result == 8);
//					assert(*((uint32_t*)(msg_buf+7)) <= 1048576);

					pthread_mutex_unlock(&in_pfs->sock_mutex);

					if (result != SOCK_DISCONNECTED && result != SOCK_ERROR)
					{
//						uint32_t numero;
//						memcpy(&numero,(msg_buf+(msg_index*msg_len))+7,4);
//						printf("entrada: %d\n",numero);
//						fflush(0);

						exit = COMM_handleReceive(msg_buf,currFD);

						free(msg_buf);
					}
					else
					{
						close(currFD);
						PFSLIST_destroyNode(in_pfs,pfsList);
						FD_CLR(currFD,&masterFDs);
					}

				}
			}
		}
	//	sem_post(&mainMutex);
		if(exit == 1){
			pfs_node_t *in_pfs = PFSLIST_getByFd(pfsList,consoleFD.descriptor);
			PFSLIST_destroyNode(in_pfs,pfsList);
			break;
		}
	}
	if(logFlag != OFF)
		log_destroy(Log);
	free(IP);
	QMANAGER_freeRequests(multiQueue->queue1);
	QUEUE_destroyQueue(multiQueue->queue1);
	if(Algorithm != SSTF)
		QMANAGER_freeRequests(multiQueue->queue2);
		QUEUE_destroyQueue(multiQueue->queue2);
	free(multiQueue);
	IO_closeDisk(file_descriptor);
	return 0;
}
