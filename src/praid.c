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

#include "tad_sockets.h"


queue_t ppdlist;
queue_t pfslist;
queue_t responselist;
uint32_t total_sectors;
pthread_mutex_t ppdlist_mutex;
pthread_mutex_t responselist_mutex;
pthread_mutex_t sync_mutex;
sem_t sync_ready_sem;
sem_t responselist_sem;
uint32_t sync_write_count;
t_log *raid_log_file;
pthread_mutex_t prueba_mutex;


int main(int argc,char **argv){

	uint32_t currFD;
	fd_set masterFDs, readFDs , PPD_FDs ,PFS_FDs;
	struct sockaddr_in remoteaddr;

	QUEUE_initialize(&ppdlist);
	QUEUE_initialize(&responselist);
	QUEUE_initialize(&pfslist);

	pthread_mutex_init(&ppdlist_mutex,NULL);
	sem_init(&sync_ready_sem,NULL,1);
	pthread_mutex_init(&responselist_mutex,NULL);

	//Inicio Leer Archivo de Configuracion
	config_param *praid_config;
	CONFIG_read("config/praid.config",&praid_config);
	//RAID_CONSOLE  = atoi(CONFIG_getValue(praid_config,"Console"));
	raid_log_file = log_create("PRAID","praid.log",DEBUG,M_CONSOLE_DISABLE);

	socketInet_t listenPFS = SOCKET_inet_create(SOCK_STREAM,"127.0.0.1",9034,MODE_LISTEN);
	sleep(1);
	socketInet_t listenPPD = SOCKET_inet_create(SOCK_STREAM,"192.168.1.107",9035,MODE_LISTEN);
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
	uint32_t FDmax=listenPPD.descriptor;   //   por ahora es este porque no hay otro
	while(1){
		FD_ZERO(&readFDs);
		readFDs = masterFDs;
		if(select(FDmax+1, &readFDs,NULL,NULL,NULL) == -1)
			perror("select");
		for(currFD = 0; currFD <= FDmax; currFD++){
			if(FD_ISSET(currFD,&readFDs)){  //hay datos nuevos
				if(currFD == listenPFS.descriptor)
				{        //nueva conexion
					uint32_t addrlen = sizeof(remoteaddr);
					uint32_t newPFS_FD;
					if((newPFS_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						if (QUEUE_length(&ppdlist) != 0)
						{
							FD_SET(newPFS_FD,&masterFDs);
							FD_SET(newPFS_FD,&PFS_FDs);
							if(newPFS_FD > FDmax)
								FDmax = newPFS_FD;

							uint32_t received = 0;
							COMM_receiveHandshake(newPFS_FD,&received);
							COMM_sendHandshake(newPFS_FD,NULL,0);

							PFSLIST_addNew(&pfslist,newPFS_FD);
						}
						else
						{
							char* handshake = malloc(4);
							memset(handshake,0,4);
							handshake[0] = HANDSHAKE;
							handshake[1] = 0x01;
							handshake[3] = 0xFF;
							send(newPFS_FD,handshake,4,0);
							close(newPFS_FD);
						}
					}
				}
				else if (currFD == listenPPD.descriptor)
				{
					uint32_t newPPD_FD,addrlen = sizeof(remoteaddr);

					if((newPPD_FD = accept(currFD,(struct sockaddr *)&remoteaddr,&addrlen))==-1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newPPD_FD,&masterFDs);
						FD_SET(newPPD_FD,&PPD_FDs);
						if(newPPD_FD > FDmax)
							FDmax = newPPD_FD;

						uint32_t received = 0;
						uint32_t len = 0;
						char *handshake = COMM_receiveWithAdvise(newPPD_FD,&received,&len);
						total_sectors = *((uint32_t*) (handshake+7));
						COMM_sendHandshake(newPPD_FD,NULL,0);
						free(handshake);
						pthread_t new_thread_id;
						pthread_mutex_lock(&ppdlist_mutex);
						pthread_create(&new_thread_id,NULL,ppd_handler_thread,NULL);
						PPDLIST_addNewPPD(newPPD_FD,new_thread_id);
						pthread_mutex_unlock(&ppdlist_mutex);
					}
				}
				else
				{

					if (FD_ISSET(currFD,&PFS_FDs))
					{
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;

						pfs_node_t	*pfs = PFSLIST_getByFd(pfslist,currFD);
						pthread_mutex_lock(&pfs->sock_mutex);
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						pthread_mutex_unlock(&pfs->sock_mutex);

						if (msg_buf != NULL)
						{
							uint32_t msg_count = dataReceived/msg_len;
							uint32_t msg_index = 0;
							if (*((uint32_t*)(msg_buf+3)) == 95)
							{
								uint32_t stop = 0;
							}
							for(;msg_index < msg_count;msg_index++)
							{
								PFSREQUEST_addNew(currFD,msg_buf+(msg_index*msg_len));
							}
							free(msg_buf);
						}
						else
						{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							FD_CLR(currFD,&PFS_FDs);
							PFSREQUEST_removeAll();
						}
					}
					else if (FD_ISSET(currFD,&PPD_FDs))
					{
						uint32_t dataReceived = 0;
						uint32_t msg_len = 0;
						ppd_node_t* ppd = PPDLIST_getByFd(ppdlist,currFD);
						pthread_mutex_lock(&ppd->sock_mutex);
						char* msg_buf = COMM_receiveWithAdvise(currFD,&dataReceived,&msg_len);
						pthread_mutex_unlock(&ppd->sock_mutex);
						if (msg_buf != NULL)
						{
							pthread_mutex_lock(&responselist_mutex);
							PFSHANDLER_sendResponse(currFD,msg_buf);
							pthread_mutex_unlock(&responselist_mutex);
							free(msg_buf);
						}
						else
						{
							close(currFD);
							FD_CLR(currFD,&masterFDs);
							FD_CLR(currFD,&PPD_FDs);
							if (QUEUE_length(&ppdlist) > 1)
							{
								PPDLIST_reorganizeRequests(currFD);
							}
							else
							{
								queueNode_t *last_ppd_node = QUEUE_takeNode(&ppdlist);
								ppd_node_t *last_ppd = (ppd_node_t*) last_ppd_node->data;
								pthread_mutex_lock(&last_ppd->request_list_mutex);
								sem_init(&last_ppd->request_list_sem,NULL,0);
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




print_Console("Adios Proceso RAID",(uint32_t)pthread_self());//CONSOLE WELCOME

return 0;
}
