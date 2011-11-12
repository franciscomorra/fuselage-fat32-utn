/*
 * praid.c
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include "config_manager.h"
#include "praid_console.h"
#include "praid_ppd_handler.h"
#include "praid_ppdlist.h"
#include "comm.h"
#include "log.h"
#include "tad_queue.h"
#include <sys/types.h>
#include <netinet/in.h>

#include "tad_sockets.h"
queue_t* pfs_list;
queue_t* ppd_list;
//QUEUE_initialize(pfs_list);
//QUEUE_initialize(ppd_list);
//datos para el select

fd_set masterFDs;					//conjunto total de FDs que queremos administrar
fd_set readFDs;
fd_set PPD_FDs;
fd_set PFS_FDs;//conjunto de FDs de los que deseamos recibir datos
uint32_t newPFS_FD;
uint32_t newPPD_FD;
uint32_t FDmax;						//mayor descriptor de socket
uint32_t consoleFD;					//FD correspondiente a la consola
uint32_t listenFD;					//FD encargado de escuchar nuevas conexiones
struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
uint32_t addrlen;
uint32_t currFD;

queue_t ppdlist;
queue_t responselist;
pthread_mutex_t ppdlist_mutex;
pthread_mutex_t responselist_mutex;
pthread_mutex_t sync_mutex;
sem_t responselist_sem;

t_log *raid_log_file;


int main(int argc,char **argv){

	QUEUE_initialize(&ppdlist);
	QUEUE_initialize(&responselist);
	pthread_mutex_init(&ppdlist_mutex,NULL);
	pthread_mutex_init(&responselist_mutex,NULL);

	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	//RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);

	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);
	sleep(1);
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9035,MODE_LISTEN);
	if (listenPPD.status != 0 || listenPFS.status != 0)
	{
		printf("ERROR AL ABRIR SOCKETS!");
		exit(0);
	}
	// Escuchar Sockets (select)
    FD_ZERO(&masterFDs);
    FD_ZERO(&PPD_FDs);
    FD_ZERO(&PFS_FDs);
	FD_SET(listenPFS.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(listenPPD.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FDmax=listenPPD.descriptor;   //   por ahora es este porque no hay otro
	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){  //hay datos nuevos
				if(currFD == listenPFS.descriptor)
				{        //nueva conexion
					addrlen = sizeof(remoteaddr);
					if((newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						if (QUEUE_length(&ppdlist) != 0)
						{
							FD_SET(newPFS_FD,&masterFDs);
							FD_SET(newPFS_FD,&PFS_FDs);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;
							char* handshake = malloc(4);
							memset(handshake,0,4);
							handshake[0] = HANDSHAKE;
							send(newPFS_FD,handshake,4,0);
						}
						else
						{
							char* handshake = malloc(4);
							memset(handshake,0,4);
							handshake[0] = HANDSHAKE;
							handshake[1] = 0x01;
							handshake[3] = 0xFF;
							send(newPFS_FD,handshake,4,0);
							close(newPFS_FD);
						}
					}
				}
				else if (currFD == listenPPD.descriptor)
				{
					addrlen = sizeof(remoteaddr);
					if((newPPD_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newPPD_FD,&masterFDs);
						FD_SET(newPPD_FD,&PPD_FDs);
						if(newPPD_FD > FDmax)
							FDmax = newPPD_FD;

						pthread_t new_thread_id;
						pthread_mutex_lock(&ppdlist_mutex);
						pthread_create(&new_thread_id,NULL,ppd_handler_thread,NULL);
						PPDLIST_addNewPPD(newPPD_FD,new_thread_id);
						pthread_mutex_unlock(&ppdlist_mutex);
					}
				}
				else
				{ //datos de un cliente
					uint32_t dataReceived;
					char *msgIn = COMM_recieve(currFD,&dataReceived);
					if (dataReceived == 0)
					{
						close(currFD);
						FD_CLR(currFD,&masterFDs);
						if (FD_ISSET(currFD,&PFS_FDs))
						{
							FD_CLR(currFD,&PFS_FDs);
							PFSREQUEST_removeAll(currFD); //TODO
						}
						else if (FD_ISSET(currFD,&PPD_FDs))
						{
							FD_CLR(currFD,&PPD_FDs);
							//TODO REORGANIZAR PEDIDOS
						}
					}
					else
					{
						if (FD_ISSET(currFD,&PFS_FDs))
						{
							PFSREQUEST_addNew(currFD,msgIn);
						}
						else if (FD_ISSET(currFD,&PPD_FDs))
						{
							pthread_mutex_lock(&responselist_mutex);
							PFSHANDLER_sendResponse(currFD,msgIn);
							pthread_mutex_unlock(&responselist_mutex);
						}
						free(msgIn);
					}
				}
			}
		}
	}




print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
