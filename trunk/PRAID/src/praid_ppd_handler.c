/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_comm.h"
#include "praid_queue.h"


extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern ppd_list_node PRAID_LIST;
extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_RAID_STATUS;

void *ppd_handler_thread (void *data)
{
	pthread_mutex_lock(&mutex_LIST);
/*
	Crea nodo PRAID_LIST
	ppd_list_node* new = malloc(sizeof(ppd_list_node));
	new->info.tid = (uint32_t)pthread_self();
	new->info.ppdStatus = 1;//SINCRONIZANDO
	SUBLISTA: Vacia
	Socket PPD: SOCKET de PPD nuevo, pasado por valor desde el main (es el void *data)

	new->next = PRAID_LIST;
	PRAID_LIST = new;
*/
	pthread_mutex_unlock(&mutex_LIST);

	while (1){
		if(RAID_STATUS != 1){ //Si es el primer PPD que se conecta, pide informacion del disco (DISK_SECTORS_AMOUNT)
			pthread_mutex_lock(&mutex_RAID_STATUS);
			RAID_STATUS = 2; //RAID ESPERANDO DISK_SECTORS_AMOUNT
			pthread_mutex_unlock(&mutex_RAID_STATUS);
		/*
			//TODO Enviar Pedido de informacion de Disco (DISK_SECTORS_AMOUNT)
			//Esperar respuesta
			//Decodificar respuesta
			//Setear DISK_SECTORS_AMOUNT
		*/
			pthread_mutex_lock(&mutex_RAID_STATUS);
			RAID_STATUS = 0; //RAID ACTIVADO
			pthread_mutex_unlock(&mutex_RAID_STATUS);

			print_Console("RAID Activado");//CONSOLE STATUS ACTIVE

		}else{ //RAID tiene al menos un disco, estado ACTIVE
			uint32_t i;
			print_Console("Nuevo PPD");//CONSOLE NEW PPD
			pthread_mutex_lock(&mutex_LIST);
			/*
				Crea nodo y lo agrega en la SUBLISTA del primer nodo de PRAID_LIST
					R/W: R
					NIPC: Primer Sector
					Socket: Socket del PPD
					Flag Pedido Synch: 1 //0-False 1-True
			*/
			pthread_mutex_unlock(&mutex_LIST);


		}
		print_Console("Iniciando Sincronizacion");//CONSOLE INICIO_SYNCHRONIZE

		while(1){
			pthread_mutex_lock(&mutex_LIST);

		/*
		Busca tu propio nodo en PRAID_LIST
		Si hay al menos un nodo en la SUBLISTA
		Lo quita de la SUBLISTA
		Lee el nodo de la SUBLISTA
		Si NIPC type = READ_SECTORS //Pedido de Lectura
			Si Synch = 0 //Pedido normal de PFS
				Envialo a PPD
				CONSOLE READ
				Espera a que vuelva
				Cuando vuelve, mandaselo al PFS directamente
			Si Synch = 1//Pedido de Synch de lectura (Insertado por un proceso nuevo de PPD)
				Envialo a PPD
				Espera a que vuelva
				Cuando vuelve
				Agregaselo a la SUBLISTA del que este SINCRONIZANDO
					R/W: W
					NIPC: Sector que leyo
					Socket: Socket del PPD
					Flag Pedido Synch: 1 //0-False 1-True
		Si NIPC type = WRITE_SECTORS //Pedido de Escritura
			Si Synch = 0//Pedido normal de PFS
				Envialo a PPD
				CONSOLE WRITE
				Espera a que vuelva
				Cuando vuelve, mandaselo al PFS directamente
			Si Synch = 1//Pedido a Synch (Solo lo deberia acceder el PPD nuevo)
				Envialo a PPD
				Espera a que vuelva
				Cuando vuelve:
					Si no es el ultimo (menor a DISK_SECTORS_AMOUNT)
						Crea nodo SUBLISTA de pedido READ y lo pone en la SUBLISTA de alguno de la PRAID_LIST
							R/W: R
							NIPC: Sector que escribio incrementado en 1
							Socket: Socket del PPD
							Flag Pedido Synch: 1 //0-False 1-True
					Si era el ultimo
						Cambia estado de PPD a ACTIVO
			*/
			pthread_mutex_unlock(&mutex_LIST);

		}

	}
return NULL;
}
