/*
 * praid.c
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <netinet/in.h>

#include "log.h"
#include "tad_sockets.h"
#include "tad_queue.h"
#include "pfs_queue.h"
#include "ppd_queue.h"
#include "ppd_thread.h"
#include "request_handler.h"
#include "config_manager.h"
#include "nipc.h"

pthread_mutex_t PPD_SYNCHRONIZING_MUTEX;
extern pthread_mutex_t REQUEST_QUEUE_MUTEX;
extern pthread_mutex_t PPD_QUEUE_MUTEX;

extern queue_t PPD_QUEUE;
extern queue_t PFS_QUEUE;
extern queue_t REQUEST_QUEUE;
extern uint32_t PPD_COUNT;
config_param *config;
t_log *raid_log;
fd_set masterFDs, readFDs , PPD_FDs ,PFS_FDs;

uint32_t pfs_handshake(uint32_t pfs_fd);
uint32_t ppd_handshake(uint32_t ppd_fd,uint32_t *disk_id,uint32_t *disk_sectors);

int main(int argc,char **argv)
{
	if (CONFIG_read("config/praid.config",&config) != 1)
	{
		printf("\nNo se encuentra o no es posible leer el archivo ./config/praid.config");
		exit(0);
	}

	uint32_t pfs_port = atoi(CONFIG_getValue(config,"PFS_PORT"));
	uint32_t ppd_port = atoi(CONFIG_getValue(config,"PPD_PORT"));
	uint32_t console_flag = strcmp(CONFIG_getValue(config,"CONSOLE"),"ON") == 0 ? 2 : 1;

	raid_log = log_create("PRAID","PRAID.log",(console_flag == M_CONSOLE_ENABLE ? INFO : OFF),console_flag);
	log_info(raid_log,"MAIN_THREAD","INICIO RAID");
	QUEUE_initialize(&PFS_QUEUE);
	QUEUE_initialize(&PPD_QUEUE);
	QUEUE_initialize(&REQUEST_QUEUE);

	PPD_COUNT = 0;
	pthread_mutex_init(&PPD_QUEUE_MUTEX,NULL);
	pthread_mutex_init(&PPD_SYNCHRONIZING_MUTEX,NULL);
	pthread_mutex_init(&REQUEST_QUEUE_MUTEX,NULL);


	uint32_t currFD;
	struct sockaddr_in remoteaddr;

	FD_ZERO(&masterFDs);
	FD_ZERO(&PPD_FDs);
	FD_ZERO(&PFS_FDs);

	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",pfs_port,MODE_LISTEN);
	sleep(1);//Porque el sleep?
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",ppd_port,MODE_LISTEN);

	// Escuchar Sockets (select)

	FD_SET(listenPFS.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(listenPPD.descriptor,&masterFDs);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	uint32_t FDmax=listenPPD.descriptor;   //   por ahora es este porque no hay otro

	while(1)
	{
		readFDs = masterFDs;

		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1){
			//print_Console("Error Select",pthread_self(),1,true);
			perror("select");
		}

		for(currFD = 0; currFD <= FDmax; currFD++)
		{
			if (FD_ISSET(currFD,&readFDs))
			{
				if(currFD == listenPFS.descriptor) //NUEVO PFS
				{
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen);

					if(newPFS_FD == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newPFS_FD,&masterFDs);
						FD_SET(newPFS_FD,&PFS_FDs);
						FDmax = newPFS_FD;
						if (pfs_handshake(newPFS_FD))
							PFSQUEUE_addNew(newPFS_FD);
					}
				}
				else if (currFD == listenPPD.descriptor) //NUEVO PPD
				{
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPPD_FD =  accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen);

					if(newPPD_FD ==-1)
					{
						perror("accept");
					}
					else
					{
						uint32_t newdisk_id = 0, newdisk_sectors = 0;
						if (ppd_handshake(newPPD_FD,&newdisk_id,&newdisk_sectors))
						{

							log_info(raid_log,"MAIN_THREAD","CONEXION NUEVO DISCO [ID: %d]",newdisk_id);
							log_info(raid_log,"PPD_THREAD","ESTADO: OPERATIVO");
							ppd_node_t *new_ppd = PPDQUEUE_addNewPPD(newPPD_FD,newdisk_id,newdisk_sectors);
							pthread_create(&new_ppd->thread_id,NULL,ppd_thread,(void*) new_ppd);
						}
						else
						{
							log_info(raid_log,"MAIN_THREAD","ERROR DE CONEXION DE DISCO - ID REPETIDO: %d",newdisk_id);
						}
					}
				}
				else //DATOS DEL PFS
				{
					char *msg_in = malloc(3);
					int32_t recvd = recv(currFD,msg_in,3,MSG_WAITALL);

				if (recvd > 0)
				{
					uint16_t msg_len = *((uint16_t*)(msg_in+1));
					msg_in = realloc(msg_in,msg_len+3);
					recv(currFD,msg_in+3,msg_len,MSG_WAITALL);

					uint32_t request_id = *((uint32_t*)(msg_in+3));
					uint32_t sector = *((uint32_t*)(msg_in+7));

					if (*msg_in == WRITE_SECTORS)
					{
						request_addNew(0,currFD,msg_in);
						pthread_mutex_lock(&PPD_QUEUE_MUTEX);
						queueNode_t *ppd_node = PPD_QUEUE.begin;
						while (ppd_node != NULL)
						{
							ppd_node_t *ppd = (ppd_node_t*) ppd_node->data;
							if (ppd->status == READY)
							{
								send(ppd->ppd_fd,msg_in,msg_len+3,MSG_WAITALL);
								ppd->requested = true;
								log_info(raid_log,"MAIN_THREAD","PEDIDO DE ESCRITURA SECTOR %d ENVIADO AL DISCO %d",sector,ppd->disk_id);
							}
							ppd_node = ppd_node->next;
						}
						pthread_mutex_unlock(&PPD_QUEUE_MUTEX);
					}
					else
					{
						ppd_node_t *selected_ppd = PPDQUEUE_selectByLessRequests();
						request_addNew(selected_ppd->ppd_fd,currFD,msg_in);
						send(selected_ppd->ppd_fd,msg_in,msg_len+3,MSG_WAITALL);
						log_info(raid_log,"MAIN_THREAD","PEDIDO DE LECTURA SECTOR %d ENVIADO AL DISCO %d",sector,selected_ppd->disk_id);
					}
				}
				else
				{
					FD_CLR(currFD,&PFS_FDs);
					FD_CLR(currFD,&masterFDs);
					PFSQUEUE_removeAll();
					////remover de la PFS LIST
				}


				}
			}
		}
	}

	return 0;
}

uint32_t pfs_handshake(uint32_t pfs_fd)
{
	extern uint32_t PPD_COUNT;
	char *handshake_msg = malloc(3);
	int32_t handshake_res = SOCKET_recvAll(pfs_fd,handshake_msg,3,0);
	if (handshake_res != SOCK_DISCONNECTED)
	{
		if (PPD_COUNT >= 1)
		{
			if (SOCKET_sendAll(pfs_fd,handshake_msg,3,0) > 0)
			{
				return 1;
			}
			return 0;
		}
		else
		{
			handshake_msg = realloc(handshake_msg,4);
			*((uint16_t*)(handshake_msg+1)) = 1;
			*(handshake_msg+3) = 0xFF;

			SOCKET_sendAll(pfs_fd,handshake_msg,4,0);
		}
	}
	free(handshake_msg);
	return 0;
}

uint32_t ppd_handshake(uint32_t ppd_fd,uint32_t *disk_id,uint32_t *disk_sectors)
{
	char *handshake_msg = malloc(11);
	int32_t handshake_res = SOCKET_recvAll(ppd_fd,handshake_msg,11,0);
	if (handshake_res != SOCK_DISCONNECTED)
	{
		*disk_id = *((uint32_t*)(handshake_msg+3));
		*disk_sectors = *((uint32_t*)(handshake_msg+7));
		free(handshake_msg);

		if (PPDQUEUE_getByID(*disk_id) == NULL)
		{
			handshake_msg = malloc(3);
			memset(handshake_msg,0,3);
			if (SOCKET_sendAll(ppd_fd,handshake_msg,3,0) > 0)
			{
				free(handshake_msg);
				return 1;
			}
		}
		else
		{
			handshake_msg = malloc(4);
			*((uint16_t*)(handshake_msg+1)) = 1;
			*(handshake_msg+3) = 0xFF;
			if (SOCKET_sendAll(ppd_fd,handshake_msg,3,0) > 0)
			{
				free(handshake_msg);
				return 0;
			}
		}
	}
	free(handshake_msg);
	return 0;
}


