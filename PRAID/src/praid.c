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

#include "log.h"

#include <sys/types.h>
#include <netinet/in.h>
//datos para el select

fd_set masterFDs;					//conjunto total de FDs que queremos administrar
fd_set readFDs;						//conjunto de FDs de los que deseamos recibir datos
uint32_t newFD;						//nuevo descriptor de socket de alguna nueva conexion
uint32_t FDmax;						//mayor descriptor de socket
uint32_t consoleFD;					//FD correspondiente a la consola
uint32_t listenFD;					//FD encargado de escuchar nuevas conexiones
struct sockaddr_in remoteaddr;		//struct correspondiente a una nueva conexion
uint32_t addrlen;
uint32_t currFD;					//current fd sirve para saber que fd tuvo cambios



uint32_t RAID_CONSOLE = 0; //0 DISABLE - 1 ENABLE
uint32_t RAID_STATUS = 0; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
uint32_t DISK_SECTORS_AMOUNT = 6; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE

praid_list_node* PRAID_LIST, CURRENT_READ;

t_log *raid_log_file;

pthread_mutex_t mutex_CONSOLE;
pthread_mutex_t mutex_RAID_STATUS;
pthread_mutex_t mutex_LIST;

int main(int argc,char **argv){

//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);
//Fin Leer archivo de Configuracion, seteada Variable Global raid_console

//Inicio Seteo de Variables Iniciales
	pthread_mutex_init(&mutex_CONSOLE, NULL);
	pthread_mutex_init(&mutex_LIST, NULL);
//Fin Seteo de Variables Iniciales

	print_Console("Inicio PRAID",(uint32_t)pthread_self());//CONSOLE WELCOME
	log_debug(raid_log_file,"PRAID","Inicio PRAID");


	pthread_t main_ppd_thread;
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,NULL);
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,NULL);
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,NULL);

	//creacion Sockets listen

	Create_Sockets_INET(&listenFD);
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
/*					if((recvReturn = recv(currFD,msgIn,519,0)) == -1)//aca hubo un error de conexión
					{
						close(currFD);
						FD_CLR(currFD,&masterFDs);
						error_fd(currFD);
					}
					else{
				   //aca tengo que preguntar el tipo del msj para saber si es PPD o PFS
						if(msgIn[10]==1){
						   //PFS=1  pongo 10 por poner un ejemplo
						   //fd_appendNode(*pfs_list,*currFD);
					       //pfs_receive(msgIn,currFD);
						   //memset(msgIn,0,sizeof(msgIn));
						}
						else if(msgIn[10]==2){
					   //PPD=2   pongo 10 por poner un ejemplo
					   fd_appendNode(*ppd_list,*currFD);
					   //ppd_receive(msgIn,currFD);
					   //memset(msgIn,0,sizeof(msgIn));
						}
					}
*/
					free(msgIn);
				}
			}
		}
	}




print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
