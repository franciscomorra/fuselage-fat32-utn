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
#include "praid_pfs_handler.h"
#include "praid_ppdlist.h"
#include "praid_pfslist.h"
#include "comm.h"
#include "log.h"
#include "tad_queue.h"
#include <sys/types.h>
#include <netinet/in.h>
#include "comm.h"
#include <assert.h>

 #include <sys/prctl.h>
#include "tad_sockets.h"


queue_t ppd_list;
queue_t pfs_list;
queue_t pending_request_list;

uint32_t sectors_perDisk;

pthread_mutex_t ppd_list_mutex;
pthread_mutex_t pending_request_list_mutex;
pthread_mutex_t sync_mutex;

sem_t pending_request_list_sem;
uint32_t pending_writes_forSyncronization;

t_log *raid_log_file;

int main(int argc,char **argv)
{
	uint32_t current_fd;
	fd_set master_fd_set, read_fd_set , PPD_fd_set ,PFS_fd_set;
	struct sockaddr_in remoteaddr;
	prctl(PR_SET_NAME,"SANTI\0",0,0,0);
	QUEUE_initialize(&ppd_list);
	QUEUE_initialize(&pending_request_list);
	QUEUE_initialize(&pfs_list);

	pthread_mutex_init(&ppd_list_mutex,NULL);
	pthread_mutex_init(&pending_request_list_mutex,NULL);

	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	//RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);

	socketInet_t listen_toPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);
	sleep(1);
	socketInet_t listen_toPPD = SOCKET_inet_create(SOCK_STREAM,"192.168.1.106",9035,MODE_LISTEN);

	if (listen_toPPD.status != 0 || listen_toPFS.status != 0)
	{
		printf("ERROR AL ABRIR SOCKETS!");
		exit(0);
	}
	// Escuchar Sockets (select)
    FD_ZERO(&master_fd_set);
    FD_ZERO(&PPD_fd_set);
    FD_ZERO(&PFS_fd_set);
	FD_SET(listen_toPFS.descriptor,&master_fd_set);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(listen_toPPD.descriptor,&master_fd_set);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	uint32_t FDmax=listen_toPPD.descriptor;
	//   por ahora es este porque no hay otro
	while(1)
	{
		FD_ZERO(&read_fd_set);
		read_fd_set = master_fd_set;

		if(select(FDmax+1,&read_fd_set,NULL,NULL,NULL) == -1)
			perror("select");

		for(current_fd = 0; current_fd <= FDmax; current_fd++)
		{
			if(FD_ISSET(current_fd,&read_fd_set))
			{  //hay datos nuevos
				if(current_fd == listen_toPFS.descriptor)
				{        //nueva conexion
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD;
					if((newPFS_FD = accept(current_fd,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						if (QUEUE_length(&ppd_list) != 0)
						{
							FD_SET(newPFS_FD,&master_fd_set);
							FD_SET(newPFS_FD,&PFS_fd_set);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;

							uint32_t received = 0;
							COMM_receiveHandshake(newPFS_FD,&received);
							COMM_sendHandshake(newPFS_FD,NULL,0);

							PFSLIST_addNew(&pfs_list,newPFS_FD);
						}
						else
						{
							char handshake = 0xFF;
							COMM_sendHandshake(newPFS_FD,&handshake,1);
							close(newPFS_FD);
						}
					}
				}
				else if (current_fd == listen_toPPD.descriptor)
				{
					uint32_t address_length = sizeof(remoteaddr);
					int32_t newPPD_FD = accept(current_fd,(struct sockaddr *)&remoteaddr,&address_length);

					if(newPPD_FD == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newPPD_FD,&master_fd_set);
						FD_SET(newPPD_FD,&PPD_fd_set);
						if(newPPD_FD > FDmax)
							FDmax = newPPD_FD;

						//MEJORAR HANDSHAKE
						uint32_t received = 0;
						uint32_t len = 0;
						char *handshake = COMM_receiveAll(newPPD_FD,&received,&len);
						sectors_perDisk = *((uint32_t*) (handshake+7));
						COMM_sendHandshake(newPPD_FD,NULL,0);
						free(handshake);
						//-----------------

						pthread_t new_thread_id;
						pthread_mutex_lock(&ppd_list_mutex);
							ppd_node_t* new_ppd = PPDLIST_addNewPPD(newPPD_FD,new_thread_id);
							pthread_create(&new_thread_id,NULL,ppd_handler_thread,(void*) new_ppd);
							pthread_setname_np(new_thread_id,"DISK");
						pthread_mutex_unlock(&ppd_list_mutex);
					}
				}
				else
				{
					if (FD_ISSET(current_fd,&PFS_fd_set))
					{
						pfs_node_t *pfs = PFSLIST_getByFd(pfs_list,current_fd);

						pthread_mutex_lock(&pfs->sock_mutex);
						char* msg_buf = malloc(3);
						int32_t result = SOCKET_recvAll(current_fd,msg_buf,3,0);
						if (result == 3)
						{
							if (result != SOCK_DISCONNECTED && result != SOCK_ERROR)
							{
								//assert(*((uint16_t*)(msg_buf+1)) == 520 || *((uint16_t*)(msg_buf+1)) == 8);
								msg_buf = realloc(msg_buf,*((uint16_t*)(msg_buf+1)) + 3);
								result = SOCKET_recvAll(current_fd,msg_buf+3,*((uint16_t*)(msg_buf+1)),0);
								pfs_request_addNew(current_fd,msg_buf);
								free(msg_buf);
							}
							else
							{
								free(msg_buf);
								close(current_fd);
								FD_CLR(current_fd,&master_fd_set);
								FD_CLR(current_fd,&PFS_fd_set);
								pfs_pending_request_removeAll();
							}
						}
						else
						{
							printf("Error: No se pudo reccibir la cabecera del mensaje IPC");
						}
						pthread_mutex_unlock(&pfs->sock_mutex);
					}
					else if (FD_ISSET(current_fd,&PPD_fd_set))
					{
						//uint32_t dataReceived = 0;
						//uint32_t msg_len = 0;
						ppd_node_t* ppd = PPDLIST_getByFd(ppd_list,current_fd);
						pthread_mutex_lock(&ppd->sock_mutex);

						char* msg_buf = malloc(3);
						int32_t result = SOCKET_recvAll(current_fd,msg_buf,3,0);

						if (result == 3)
						{
							//assert(*((uint16_t*)(msg_buf+1)) == 520 || *((uint16_t*)(msg_buf+1)) == 8);
							msg_buf = realloc(msg_buf,*((uint16_t*)(msg_buf+1)) + 3);
							result = SOCKET_recvAll(current_fd,msg_buf+3,*((uint16_t*)(msg_buf+1)),0);
							//assert(result == 520);

							/*if (*msg_buf == 0x02)
							{
								printf("IN W%d\n",*((uint32_t*) (msg_buf+7)));
							}
							else if (*msg_buf == 0x01)
							{
								printf("IN R%d\n",*((uint32_t*) (msg_buf+7)));
							}
							else
							{
								uint32_t error = 0;
							}*/

							pthread_mutex_unlock(&ppd->sock_mutex);

							if (result != SOCK_DISCONNECTED && result != SOCK_ERROR)
							{
								//assert(*((uint32_t*)(msg_buf+7)) <= 1048576);

								pfs_pending_request_attendTo(current_fd,msg_buf);

								free(msg_buf);
							}
							else
							{
								close(current_fd);
								FD_CLR(current_fd,&master_fd_set);
								FD_CLR(current_fd,&PPD_fd_set);

								if (QUEUE_length(&ppd_list) > 1)
								{
									PPDLIST_reorganizeRequests(current_fd);
								}
								else
								{
									//TERMINAR THREADS
									queueNode_t *last_ppd_node = QUEUE_takeNode(&ppd_list);
									ppd_node_t *last_ppd = (ppd_node_t*) last_ppd_node->data;
									pthread_mutex_lock(&last_ppd->request_list_mutex);
									sem_init(&last_ppd->request_list_sem,0,0);
									queueNode_t *cur_request_node;
									while ((cur_request_node = QUEUE_takeNode(&last_ppd->request_list)) != NULL)
									{
										pfs_request_t *cur_request = (pfs_request_t*) cur_request_node->data;
										free(cur_request->msg);
										free(cur_request_node->data);
										free(cur_request_node);
									}
									pthread_mutex_unlock(&last_ppd->request_list_mutex);
									free(last_ppd_node->data);
									free(last_ppd_node);
								}
							}
						}
						else
						{
							printf("Error: No se pudo reccibir la cabecera del mensaje IPC");
						}
					}
				}
			}
		}
	}




print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
