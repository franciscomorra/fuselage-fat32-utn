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

#include "config_manager.h"
#include "praid_console.h"
#include "praid_ppd_handler.h"
#include "praid_queue.h"

uint32_t RAID_CONSOLE = 0; //0 DISABLE - 1 ENABLE
uint32_t RAID_STATUS = 0; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE

praid_list_node* PRAID_LIST;

pthread_mutex_t mutex_CONSOLE;
pthread_mutex_t mutex_RAID_STATUS;
pthread_mutex_t mutex_LIST;

int main(int argc,char **argv){

//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
//Fin Leer archivo de Configuracion, seteada Variable Global raid_console

//Inicio Seteo de Variables Iniciales
	pthread_mutex_init(&mutex_CONSOLE, NULL);
	pthread_mutex_init(&mutex_LIST, NULL);
//Fin Seteo de Variables Iniciales

	print_Console("Bienvenido Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME
	//Inicio Creacion Sockets
	//TODO Crear Sockets
	//Fin Creacion Sockets

	//while(1){
		//TODO Escuchar Sockets
		/*
		Si es de PFS
			receive_pfs(nipcMsg_t msgIn)
		Si es de nuevo PPD
			pthread_t praid_ppd_thread;
			pthread_create(&praid_ppd_thread, NULL, ppd_handler_thread, SOCKET DE PPD NUEVO!);
		*/
	//}

//Inicio Liberar PRAID_LIST y variables
	pthread_mutex_destroy(&mutex_CONSOLE);
	pthread_mutex_destroy(&mutex_LIST);
	print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
