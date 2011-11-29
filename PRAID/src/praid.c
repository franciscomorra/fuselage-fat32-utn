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
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "config_manager.h"
#include "praid_console.h"
#include "praid_ppd_handler.h"
#include "praid_queue.h"
#include "praid_comm.h"
#include "comm.h"
#include "log.h"
#include "tad_queue.h"
#include "tad_sockets.h"

t_log *raid_log_file;

uint32_t RAID_CONSOLE = 0; //0 DISABLE - 1 ENABLE
uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
uint32_t ACTIVE_DISKS_AMOUNT;
uint32_t DISKS_AMOUNT;

uint32_t LOG_ENABLE = 0;
bool RAID_ACTIVE;
bool SYNCHRONIZING_DISCS;

queue_t* WRITE_QUEUE;
praid_list_node* PRAID_LIST, CURRENT_READ;
t_log *raid_log_file;

pthread_mutex_t mutex_CONSOLE;
pthread_mutex_t mutex_LIST;
pthread_mutex_t mutex_WRITE_QUEUE;
fd_set masterFDs, readFDs , PPD_FDs ,PFS_FDs;

int main(int argc,char **argv){
	uint32_t currFD;
	struct sockaddr_in remoteaddr;
	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	uint32_t PPD_Port = atoi(CONFIG_getValue(praid_config,"PpdPort"));
	uint32_t PFS_Port = atoi(CONFIG_getValue(praid_config,"PfsPort"));
	LOG_ENABLE= atoi(CONFIG_getValue(praid_config,"Log"));//TRUE = 1;FALSE = 0;
	if(LOG_ENABLE == 1){
		raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);
	}
	//Fin Leer archivo de Configuracion


	//Inicio Seteo de Variables Iniciales
	pthread_mutex_init(&mutex_CONSOLE, NULL);
	pthread_mutex_init(&mutex_WRITE_QUEUE, NULL);
	pthread_mutex_init(&mutex_LIST, NULL);
	WRITE_QUEUE = malloc(sizeof(queue_t));
	ACTIVE_DISKS_AMOUNT = 0;
	DISKS_AMOUNT = 0;
	DISK_SECTORS_AMOUNT = 0;
	RAID_ACTIVE = false;
	SYNCHRONIZING_DISCS = false;
	//QUEUE_initialize(WRITE_QUEUE);
	//Fin Seteo de Variables Iniciales

	print_Console("INICIO PROCESO RAID",(uint32_t)pthread_self(),1,true);//CONSOLE WELCOME
	PRAID_WRITE_LOG("Inicio Proceso RAID");
	//log_debug(raid_log_file,"PRAID","Inicio Proceso RAID");

	print_Console("ABRIENDO PUERTOS...",(uint32_t)pthread_self(),1,false);//CONSOLE WELCOME
	char* ip_ppd = "127.0.0.1";
	sleep(1);
	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"NULL",PFS_Port,MODE_LISTEN);
	sleep(1);//Porque el sleep?
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,ip_ppd,PPD_Port,MODE_LISTEN);
	if (listenPPD.status != 0 || listenPFS.status != 0)
	{
		print_Console("ERROR ABRIENDO PUERTOS",0,1,false);
		print_Console("ESTADO PUERTO PPD:",listenPPD.status,1,true);
		print_Console("ESTADO PUERTO PFS:",listenPFS.status,1,true);
		exit(0);
	}else{
		print_Console("PUERTOS ABIERTOS",pthread_self(),1,false);
		print_Console(ip_ppd,pthread_self(),1,false);
		print_Console("ESPERANDO CONEXION DE DISCOS...",0,1,false);

		//free(ip_ppd);
	}
	// Escuchar Sockets (select)
    FD_ZERO(&masterFDs);
    FD_ZERO(&PPD_FDs);
    FD_ZERO(&PFS_FDs);
	FD_SET(listenPFS.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(listenPPD.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	uint32_t FDmax=listenPPD.descriptor;   //   por ahora es este porque no hay otro
	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1){
			//print_Console("Error Select",pthread_self(),1,true);
			perror("select");
		}
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){  //hay datos nuevos
				if(currFD == listenPFS.descriptor){//CONEXION PFS NUEVA
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD;
					if((newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1){
						perror("accept");
						print_Console("ERROR CONEXION DE PFS, SOCKET",newPFS_FD,1,true);
					}else{
						print_Console("NUEVA CONEXION DE PFS, SOCKET:",newPFS_FD,1,true);
						if (RAID_ACTIVE == true){
							FD_SET(newPFS_FD,&masterFDs);
							FD_SET(newPFS_FD,&PFS_FDs);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;
							uint32_t received = 0;
							COMM_receiveHandshake(newPFS_FD,&received);
							COMM_sendHandshake(newPFS_FD,NULL,0);
						}else{
							COMM_sendErrorHandshake(newPFS_FD);
							print_Console("CONEXION DE PFS DENEGADA RAID INACTIVO",pthread_self(),1,true);
						}
					}
				}else if (currFD == listenPPD.descriptor){
					uint32_t newPPD_FD,addrlen = sizeof(remoteaddr);
					if((newPPD_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1){
						print_Console("ERROR CONEXION DE DISCO, SOCKET",newPPD_FD,1,true);
						perror("accept");
					}else{
						print_Console("NUEVA CONEXION DE DISCO, SOCKET:",newPPD_FD,1,true);
						uint32_t received = 0;
						uint32_t len = 0;
						char *handshake = COMM_receiveWithAdvise(newPPD_FD,&received,&len);
						praid_ppdThreadParam* ppdParam = PRAID_ValidatePPD(handshake,newPPD_FD);//Valida datos del ppd (Cant Sectores e ID)
						if(ppdParam!=NULL){//Si el handshake estaba ok
							FD_SET(newPPD_FD,&masterFDs);
							FD_SET(newPPD_FD,&PPD_FDs);
							if(newPPD_FD > FDmax){
								FDmax = newPPD_FD;
							}
							pthread_t main_ppd_thread;
							COMM_sendHandshake(newPPD_FD,NULL,0);
							pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)ppdParam);
						}else{//El handshake vino con errores
							COMM_sendErrorHandshake(newPPD_FD);
							print_Console("CONEXION DE DISCO DENEGADA",newPPD_FD,1,true);
						}

					}
				}else{
					if (FD_ISSET(currFD,&PFS_FDs)){
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						print_Console("Recibida informacion de un PFS existente",currFD,1,true);

						if (msg_buf != NULL){
							PRAID_PFS_RECEIVE_REQUEST(msg_buf,currFD);//Recibi del PFS
							print_Console("El pedido de PFS no es vacio",currFD,1,true);
						}else{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							FD_CLR(currFD,&PFS_FDs);
							print_Console("PFS DESCONECTADO - FIN",pthread_self(),1,true);
							PRAID_WRITE_LOG("Apagado Proceso RAID, PFS desconectado");
							exit(0);
						}
						free(msg_buf);
					}else if (FD_ISSET(currFD,&PPD_FDs)){
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						if (msg_buf != NULL){
							pthread_mutex_lock(&mutex_LIST);
							PRAID_PPD_RECEIVE_REQUEST(msg_buf,currFD);//Actualiza en el thread PPD
							pthread_mutex_unlock(&mutex_LIST);
							free(msg_buf);
							/*
							if(msg_buf[0]==HANDSHAKE){//EL ERROR PUTO QUE ME ESTA TIRANDO AHORA, PPD SE BAJA CUANDO RECIBE EL HAND Y ME MANDA OTRO HAND?
								print_Console("***ERROR - DOBLE HANSHAKE***",currFD,1,true);
								close(currFD);
								FD_CLR(currFD,&masterFDs);
								FD_CLR(currFD,&PPD_FDs);
								sleep(2);
								pthread_mutex_lock(&mutex_LIST);
								PRAID_HANDLE_DOWN_PPD(currFD);
								pthread_mutex_unlock(&mutex_LIST);

							}*/
						}else{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							if (FD_ISSET(currFD,&PFS_FDs)){//PFS de baja
								FD_CLR(currFD,&PFS_FDs);
								print_Console("PFS DESCONECTADO - FIN",pthread_self(),1,true);
								exit(0);
							}else if (FD_ISSET(currFD,&PPD_FDs)){
								FD_CLR(currFD,&PPD_FDs); // PPD de baja
								pthread_mutex_lock(&mutex_LIST);
								PRAID_HANDLE_DOWN_PPD(currFD);
								pthread_mutex_unlock(&mutex_LIST);
							}
						}
						//free(msg_buf);
					}
				}
			}
		}
	}
	print_Console("Adios Proceso RAID",(uint32_t)pthread_self(),1,true);//CONSOLE WELCOME
	return 0;
}
