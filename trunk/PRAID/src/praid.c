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
/*
queue_t* pfs_list;
queue_t* ppd_list;
*/
//datos para el select
/*
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
*/


t_log *raid_log_file;

uint32_t RAID_CONSOLE = 0; //0 DISABLE - 1 ENABLE
bool RAID_ACTIVE = false;
uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
queue_t* WRITE_QUEUE;
bool SYNCHRONIZING_DISCS = false;

praid_list_node* PRAID_LIST, CURRENT_READ;

t_log *raid_log_file;

pthread_mutex_t mutex_CONSOLE;
pthread_mutex_t mutex_LIST;
pthread_mutex_t mutex_WRITE_QUEUE;

int main(int argc,char **argv){
	uint32_t currFD;
	fd_set masterFDs, readFDs , PPD_FDs ,PFS_FDs;
	struct sockaddr_in remoteaddr;
	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	uint32_t PPD_Port = atoi(CONFIG_getValue(praid_config,"PpdPort"));
	uint32_t PFS_Port = atoi(CONFIG_getValue(praid_config,"PfsPort"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);
	//Fin Leer archivo de Configuracion, seteada Variable Global raid_console

	//Inicio Seteo de Variables Iniciales
	pthread_mutex_init(&mutex_CONSOLE, NULL);
	pthread_mutex_init(&mutex_WRITE_QUEUE, NULL);
	pthread_mutex_init(&mutex_LIST, NULL);
	WRITE_QUEUE = malloc(sizeof(queue_t));
	//QUEUE_initialize(WRITE_QUEUE);
	//Fin Seteo de Variables Iniciales

	print_Console("INICIO PROCESO RAID",(uint32_t)pthread_self(),1,true);//CONSOLE WELCOME

	log_debug(raid_log_file,"PRAID","Inicio Proceso RAID");
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
				if(currFD == listenPFS.descriptor)//CONEXION PFS NUEVA
				{
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD;
					if((newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
						print_Console("Error Conexion PFS",newPFS_FD,1,true);
					}
					else
					{
						print_Console("Nueva conexion PFS",newPFS_FD,1,true);
						if (RAID_ACTIVE == true){
							FD_SET(newPFS_FD,&masterFDs);
							FD_SET(newPFS_FD,&PFS_FDs);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;
							uint32_t received = 0;
							COMM_receiveHandshake(newPFS_FD,&received);
							COMM_sendHandshake(newPFS_FD,NULL,0);
							print_Console("Conexion PFS aceptada",newPFS_FD,1,true);
						}else{

							char* handshake = malloc(4);
							memset(handshake,0,4);
							handshake[0] = HANDSHAKE;
							handshake[1] = 0x01;
							handshake[3] = 0xFF;
							send(newPFS_FD,handshake,4,0);
							close(newPFS_FD);
							print_Console("Conexion PFS denegada, no hay discos",pthread_self(),1,true);
							free(handshake);
						}
					}
				}
				else if (currFD == listenPPD.descriptor)
				{
					uint32_t newPPD_FD,addrlen = sizeof(remoteaddr);
					if((newPPD_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						print_Console("ERROR CONEXION DE DISCO, SOCKET",newPPD_FD,1,true);
						perror("accept");
					}
					else
					{
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
							char* handshake = malloc(4);
							memset(handshake,0,4);
							handshake[0] = HANDSHAKE;
							handshake[1] = 0x01;
							handshake[3] = 0xFF;
							send(newPPD_FD,handshake,4,0);
							close(newPPD_FD);
							free(handshake);
							print_Console("CONEXION DE DISCO DENEGADA",newPPD_FD,1,true);
						}

					}
				}
				else
				{
					if (FD_ISSET(currFD,&PFS_FDs))
					{
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						print_Console("Recibida informacion de un PFS existente",currFD,1,true);

						if (msg_buf != NULL)
						{
							PRAID_PFS_RECEIVE_REQUEST(msg_buf,currFD);//Recibi del PFS
							print_Console("El pedido de PFS no es vacio",currFD,1,true);
						}
						else
						{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							FD_CLR(currFD,&PFS_FDs);
							print_Console("PFS DESCONECTADO - FIN",pthread_self(),1,true);
							log_debug(raid_log_file,"PRAID","Apagado Proceso RAID, PFS desconectado");
							//Antes envia todos los pedidos!
							exit(0);
						}
						free(msg_buf);

					}
					else if (FD_ISSET(currFD,&PPD_FDs))
					{
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						//print_Console("PPD EXISTENTE ENVIO RESPUESTA",currFD);
						//print_Console("CANTIDAD DE DISCOS ACTIVOS: ",PRAID_ACTIVE_PPD_COUNT());


						if (msg_buf != NULL)
						{
							if(msg_buf[0]==HANDSHAKE){//EL ERROR PUTO QUE ME ESTA TIRANDO AHORA, PPD SE BAJA CUANDO RECIBE EL HAND

								print_Console("***ERROR - DOBLE HANSHAKE***",currFD,1,false);
								close(currFD);
								FD_CLR(currFD,&masterFDs);
								FD_CLR(currFD,&PPD_FDs);
								sleep(2);
								pthread_mutex_lock(&mutex_LIST);
								praid_list_node* nodoBuscado = PRAID_GET_PPD_FROM_FD(currFD);

								if(nodoBuscado!=NULL){
									nodoBuscado->ppdStatus = DISCONNECTED;
									sem_post(&nodoBuscado->request_list_sem);
								}
								pthread_mutex_unlock(&mutex_LIST);

							}else{
								//print_Console("La respuesta de PPD no es vacia",currFD);
								pthread_mutex_lock(&mutex_LIST);
								PRAID_PPD_RECEIVE_REQUEST(msg_buf,currFD);//Actualiza en el thread PPD
								pthread_mutex_unlock(&mutex_LIST);
							}
						}
						else
						{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							if (FD_ISSET(currFD,&PFS_FDs)){//PFS de baja
								FD_CLR(currFD,&PFS_FDs);

							}else if (FD_ISSET(currFD,&PPD_FDs)){
								FD_CLR(currFD,&PPD_FDs); // PPD de baja

								pthread_mutex_lock(&mutex_LIST);
								praid_list_node* nodoBuscado = PRAID_GET_PPD_FROM_FD(currFD);
								print_Console("DISCO DESCONECTADO",nodoBuscado->tid,1,false);

								if(PRAID_ACTIVE_PPD_COUNT()==1 && nodoBuscado->ppdStatus== READY){
									print_Console("UNICO DISCO DESCONECTADO",pthread_self(),1,false);
									log_debug(raid_log_file,"PRAID","Apagado Proceso RAID, Unico PPD desconectado");
									exit(0);//TODO QUE SE DESACTIVE
								}
								if(nodoBuscado->ppdStatus == READY && PRAID_ACTIVE_PPD_COUNT()==2 && SYNCHRONIZING_DISCS==true){
									print_Console("MASTER DESCONECTADO MIENTRAS SINCRONIZABA -- APAGANDO PROCESO",pthread_self(),1,false);
									log_debug(raid_log_file,"PRAID","Apagado Proceso RAID, Master desconectado mientras sincronizaba");
									//ERROR GRAVE, SI SE QUIERE, QUE AGARRE EL PRIMER DISCO Y VACIE LA COLA DE PEDIDOS DEL PFS
									exit(0);
								}
								if(nodoBuscado->ppdStatus == SYNCHRONIZING){
									SYNCHRONIZING_DISCS = false;
									if(PRAID_ACTIVATE_NEXT_SYNCH()==false){
										print_Console("NO HAY MAS DISCOS PARA SINCRONIZAR",pthread_self(),1,false);
									}else{
										print_Console("ACTIVANDO DISCO A SINCRONIZAR",nodoBuscado->tid,1,true);
									}
								}

								nodoBuscado->ppdStatus = DISCONNECTED;
								sem_post(&nodoBuscado->request_list_sem);
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
