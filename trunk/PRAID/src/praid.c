/*
 * praid.c
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

#include "config_manager.h"

#include "praid_console.h"
#include "praid_listener.h"
#include "praid_queue.h"
#include "tad_queue.h"

uint32_t raid_console = 0; //0 ENABLE - 1 DISABLE
uint32_t raid_status = 0; //0 INACTIVE - 1 ACTIVE
uint32_t ppd_thread_amount = 0; // CONTADOR DE THREADS DE PPD
read_node read_first;
read_node read_last;
write_node write_first;
write_node write_last;

pthread_mutex_t mutex_console;
pthread_mutex_t mutex_READ;
pthread_mutex_t mutex_WRITE;

int main(int argc,char **argv){

/*
	//TODO Leer Archivo de configuracion Y VER SI CONSOLE ENABLE O DISABLE
		config_param *praid_config;
		CONFIG_read("config/praid.config",&praid_config);
		raid_console  = atoi(CONFIG_getValue(praid_config,"Console"));
	//Error de make. Creo que es el linkeo de Commons, que no encuentra el .c
*/

	pthread_mutex_init(&mutex_console, NULL);
	pthread_mutex_init(&mutex_READ, NULL);
	pthread_mutex_init(&mutex_WRITE, NULL);

	print_Console("Bienvenido Proceso RAID");

	//TODO Crear Cola Read y Write

	queue_t colaREAD;
	QUEUE_initialize(&colaREAD);
	queue_t colaWrite;
	QUEUE_initialize(&colaWrite);


	pthread_t listener_thread;
	pthread_create(&listener_thread, NULL, praid_listener, NULL);
	pthread_join(listener_thread, NULL);

	//TODO Destruir Cola Read y Write, recorrerla y eliminar nodos

	pthread_mutex_destroy(&mutex_console);
	pthread_mutex_destroy(&mutex_READ);
	pthread_mutex_destroy(&mutex_WRITE);



return 0;
}
