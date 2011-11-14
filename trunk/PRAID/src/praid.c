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
#include "praid_queue.h"
#include "praid_comm.h"
#include "comm.h"
#include "log.h"
#include "tad_queue.h"
#include <sys/types.h>
#include <netinet/in.h>

#include "tad_sockets.h"
#include <stdbool.h>

queue_t* pfs_list;
queue_t* ppd_list;

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

t_log *raid_log_file;

uint32_t RAID_CONSOLE = 0; //0 DISABLE - 1 ENABLE
bool RAID_ACTIVE = false;
uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
queue_t* WRITE_QUEUE;

praid_list_node* PRAID_LIST, CURRENT_READ;

t_log *raid_log_file;

pthread_mutex_t mutex_CONSOLE;
pthread_mutex_t mutex_LIST;
pthread_mutex_t mutex_WRITE_QUEUE;

int main(int argc,char **argv){

//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));

	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);
//Fin Leer archivo de Configuracion, seteada Variable Global raid_console

//Inicio Seteo de Variables Iniciales
	pthread_mutex_init(&mutex_CONSOLE, NULL);
	pthread_mutex_init(&mutex_WRITE_QUEUE, NULL);
	pthread_mutex_init(&mutex_LIST, NULL);
	WRITE_QUEUE = malloc(sizeof(queue_t));
	//QUEUE_initialize(WRITE_QUEUE);
//Fin Seteo de Variables Iniciales

	print_Console("Inicio PRAID",(uint32_t)pthread_self());//CONSOLE WELCOME
	log_debug(raid_log_file,"PRAID","Inicio Proceso RAID");


	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);//TODO poner en Config (IP??)
	sleep(1);//Porque el sleep?
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9035,MODE_LISTEN);//TODO poner en Config
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
				if(currFD == listenPFS.descriptor){ //nueva conexion
					addrlen = sizeof(remoteaddr);
					if((newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1){
						perror("accept");
					}else{
						char* msgOut = malloc(4);
						memset(msgOut,0,4);
						msgOut[0] = HANDSHAKE; //Nuevo PFS
						if (RAID_ACTIVE == true){
							FD_SET(newPFS_FD,&masterFDs);
							FD_SET(newPFS_FD,&PFS_FDs);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;
							send(newPFS_FD,msgOut,4,0);
						}else{
							msgOut[1] = 0x01;
							msgOut[3] = 0xFF;
							send(newPFS_FD,msgOut,4,0);
							close(newPFS_FD);
						}
						free(msgOut);
					}
				}else if (currFD == listenPPD.descriptor){
					addrlen = sizeof(remoteaddr);
					if((newPPD_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1){
						perror("accept");
					}else{//Nuevo PPD
						uint32_t dataReceived;
						char *msgIn = COMM_recieve(currFD,&dataReceived);

						//TODO OBTENER CANTIDAD DE SECTORES DISK_SECTORS_AMOUNT QUE ESTA EN EL HANDSHAKE

						free(msgIn);

						FD_SET(newPPD_FD,&masterFDs);
						FD_SET(newPPD_FD,&PPD_FDs);
						if(newPPD_FD > FDmax)
							FDmax = newPPD_FD;

						char* msgOut = malloc(4);
						memset(msgOut,0,4);
						msgOut[0] = HANDSHAKE;
						send(newPPD_FD,msgOut,4,0);

						pthread_t main_ppd_thread;
						pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)newPPD_FD);

					}
				}else{ //datos de cliente existente
					uint32_t dataReceived;
					char *msgIn = COMM_recieve(currFD,&dataReceived);
					if (dataReceived == 0){
						close(currFD);
						FD_CLR(currFD,&masterFDs);
						if (FD_ISSET(currFD,&PFS_FDs)){
							FD_CLR(currFD,&PFS_FDs);//PFS de baja
							print_Console("Adios Proceso RAID",pthread_self());
							log_debug(raid_log_file,"PRAID","Apagado Proceso RAID");

							return 1;
							//Se cae el raid, se tiene que apagar
						}else if (FD_ISSET(currFD,&PPD_FDs)){
							FD_CLR(currFD,&PPD_FDs); // PPD de baja

							praid_list_node* nodoBuscado = PRAID_SearchPPDBySocket(currFD);
							print_Console("PPD Desconectado",nodoBuscado->tid);

							nodoBuscado->ppdStatus = DISCONNECTED;
						}
					}else{
						if (FD_ISSET(currFD,&PFS_FDs)){
							PRAID_pfs_receive(msgIn,currFD);
						}else if (FD_ISSET(currFD,&PPD_FDs)){
							PRAID_ppd_receive(msgIn,currFD);
						}
						free(msgIn);
					}
				}
			}
		}
	}

return 0;
}













//creacion Sockets listen
/*
socketInet_t sock_listen = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);
listenFD = sock_listen.descriptor;
// Escuchar Sockets (select)
FD_ZERO(&masterFDs);
FD_SET(listenFD,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
FDmax=listenFD;   //   por ahora es este porque no hay otro
while(1){
	FD_ZERO(&readFDs);
	readFDs = masterFDs;
	if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
		perror("select");
	for(currFD = 0; currFD <= FDmax; currFD++){
		if(FD_ISSET(currFD,&readFDs)){  //hay datos nuevos
			if( currFD == listenFD ){        //nueva conexion
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
				char* msgIn = malloc(512 + 7);
				if((recvReturn = recv(currFD,msgIn,519,0)) == 0)//aca el cliente cerro conexion
				{
					close(currFD);
					FD_CLR(currFD,&masterFDs);
				}
				else{
			   //aca tengo que preguntar el tipo del msj para saber si es PPD o PFS
					if(msgIn[3]==1)    //PFS=1
					{
					   pfs_receive(msgIn,currFD);
					   memset(msgIn,0,sizeof(msgIn));
					}
					else if(msgIn[3]==2) //PPD=2
					{
				     ppd_receive(msgIn,currFD);
				     memset(msgIn,0,sizeof(msgIn));
					}
				}

				free(msgIn);
			}
		}
	}
}

*/
