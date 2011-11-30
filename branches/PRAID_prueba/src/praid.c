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


#include "tad_sockets.h"


queue_t ppdlist;
queue_t pfslist;
queue_t responselist;

uint32_t total_sectors;

pthread_mutex_t ppdlist_mutex;
pthread_mutex_t responselist_mutex;
pthread_mutex_t sync_mutex;

sem_t responselist_sem;
uint32_t sync_write_count;
t_log *raid_log_file;



int main(int argc,char **argv){


	uint32_t current_fd;
	fd_set master_fd_set, read_fd_set , PPD_fd_set ,PFS_fd_set;
	struct sockaddr_in remoteaddr;

	QUEUE_initialize(&ppdlist);
	QUEUE_initialize(&responselist);
	QUEUE_initialize(&pfslist);

	pthread_mutex_init(&ppdlist_mutex,NULL);
	pthread_mutex_init(&responselist_mutex,NULL);

	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	//RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);

	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);
	sleep(1);
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,"192.168.1.106",9035,MODE_LISTEN);

	if (listenPPD.status != 0 || listenPFS.status != 0)
	{
		printf("ERROR AL ABRIR SOCKETS!");
		exit(0);
	}
	// Escuchar Sockets (select)
    FD_ZERO(&master_fd_set);
    FD_ZERO(&PPD_fd_set);
    FD_ZERO(&PFS_fd_set);
	FD_SET(listenPFS.descriptor,&master_fd_set);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	FD_SET(listenPPD.descriptor,&master_fd_set);  //agrego el descriptor que recibe conexiones al conjunto de FDs
	uint32_t FDmax=listenPPD.descriptor;		//   por ahora es este porque no hay otro
	while(1)
	{
		FD_ZERO(&read_fd_set);
		read_fd_set = master_fd_set;

		if(select(FDmax+1,&read_fd_set,NULL,NULL,NULL) == -1)
			perror("select");

		for(current_fd = 0; current_fd <= FDmax; current_fd++)
		{
			if(FD_ISSET(current_fd,&read_fd_set)){  //hay datos nuevos
				if(current_fd == listenPFS.descriptor)
				{        //nueva conexion
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD;
					if((newPFS_FD = accept(current_fd,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						if (QUEUE_length(&ppdlist) != 0)
						{
							FD_SET(newPFS_FD,&master_fd_set);
							FD_SET(newPFS_FD,&PFS_fd_set);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;

							uint32_t received = 0;
							COMM_receiveHandshake(newPFS_FD,&received);
							COMM_sendHandshake(newPFS_FD,NULL,0);

							PFSLIST_addNew(&pfslist,newPFS_FD);
						}
						else
						{
							char handshake = 0xFF;
							COMM_sendHandshake(newPFS_FD,&handshake,1);
							close(newPFS_FD);
						}
					}
				}
				else if (current_fd == listenPPD.descriptor)
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

						uint32_t received = 0;
						uint32_t len = 0;
						char *handshake = COMM_receiveAll(newPPD_FD,&received,&len);
						total_sectors = *((uint32_t*) (handshake+7));
						COMM_sendHandshake(newPPD_FD,NULL,0);
						free(handshake);

						pthread_t new_thread_id;
						pthread_mutex_lock(&ppdlist_mutex);
							ppd_node_t* new_ppd = PPDLIST_addNewPPD(newPPD_FD,new_thread_id);
							pthread_create(&new_thread_id,NULL,ppd_handler_thread,(void*) new_ppd);
						pthread_mutex_unlock(&ppdlist_mutex);
					}
				}
				else
				{
					if (FD_ISSET(current_fd,&PFS_fd_set))
					{
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;

						pfs_node_t	*pfs = PFSLIST_getByFd(pfslist,current_fd);

						pthread_mutex_lock(&pfs->sock_mutex);
							char* msg_buf = COMM_receiveAll(current_fd,&dataReceived,&msg_len);
						pthread_mutex_unlock(&pfs->sock_mutex);

						if (msg_buf != NULL)
						{
							uint32_t msg_count = dataReceived/msg_len;
							uint32_t msg_index = 0;

							for(;msg_index < msg_count;msg_index++)
							{
								PFSREQUEST_addNew(current_fd,msg_buf+(msg_index*msg_len));
							}
							free(msg_buf);
						}
						else
						{
							close(current_fd);
							FD_CLR(current_fd,&master_fd_set);
							FD_CLR(current_fd,&PFS_fd_set);
							PFSREQUEST_removeAll();
						}
					}
					else if (FD_ISSET(current_fd,&PPD_fd_set))
					{
						//uint32_t dataReceived = 0;
						//uint32_t msg_len = 0;
						ppd_node_t* ppd = PPDLIST_getByFd(ppdlist,current_fd);
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
								pthread_mutex_lock(&responselist_mutex);
									PFSHANDLER_sendResponse(current_fd,msg_buf);
								pthread_mutex_unlock(&responselist_mutex);
								free(msg_buf);
							}
							else
							{
								close(current_fd);
								FD_CLR(current_fd,&master_fd_set);
								FD_CLR(current_fd,&PPD_fd_set);
								if (QUEUE_length(&ppdlist) > 1)
								{
									PPDLIST_reorganizeRequests(current_fd);
								}
								else
								{
									//TERMINAR THREADS
									queueNode_t *last_ppd_node = QUEUE_takeNode(&ppdlist);
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
					}
				}
			}
		}
	}




print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
