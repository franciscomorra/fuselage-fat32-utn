#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "nipc.h"
#include "ppd_SSTF.h"
#include "ppd_comm.h"
#include "tad_queue.h"
#include "ppd_common.h"
#include "ppd_translate.h"
#include "ppd_qManager.h"
#include "tad_sockets.h"
#include "comm.h"

#define PORT 9034

extern multiQueue_t* multiQueue;
extern sem_t mainMutex;
extern uint32_t headPosition;

uint32_t COMM_handleReceive(char* msgIn,uint32_t fd) {

	switch (msgIn[0]) {
		case HANDSHAKE:
				//TODO Handshake
			break;

		case PPDCONSOLE_INFO:{
			CHS_t* CHSPosition = COMMON_turnToCHS(headPosition);

			memcpy(msgIn+3,&CHSPosition->cylinder,4);
			memcpy(msgIn+7,&CHSPosition->head,4);
			memcpy(msgIn+11,&CHSPosition->sector,4);
			COMM_send(msgIn,fd);

			free(CHSPosition);
			break;
		}

		default:{ // puede ser tanto de lectura, escritura o de tipo trace

			request_t* request = TRANSLATE_fromCharToRequest(msgIn,fd);
			queue_t* queue = QMANAGER_selectPassiveQueue(multiQueue);

			sem_wait(&mainMutex);
			QUEUE_appendNode(queue,request);
			sem_post(&mainMutex);

			sem_post(&multiQueue->queueElemSem);

			break;
		}
	}
			return 0;
}

uint32_t COMM_connect(uint32_t* listenFD){

	struct sockaddr_in myaddr;

	if((*listenFD = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(PORT);
	memset(&(myaddr.sin_zero),'\0',8);

	if(bind(*listenFD,(struct sockaddr *)&myaddr,sizeof(myaddr))==-1){
		perror("bind");
		exit(1);
	}
	if(listen(*listenFD,10) == -1){
		perror("listen");
		exit(1);
	}

	return 0;
}

char* COMM_createCharMessage(NIPC_type type,uint32_t payload_bytes_len)
{
	char* msg = malloc(payload_bytes_len + 3);

	msg[0] = type;
	memcpy(msg+1,&payload_bytes_len,2);
	memset(msg+3,0,payload_bytes_len);
	return msg;
}
socketUnix_t COMM_ConsoleAccept(socketUnix_t connect){

	struct sockaddr_un remote;
	socketUnix_t console_socket;

	console_socket.style = SOCK_STREAM;
	console_socket.path = malloc(strlen(connect.path));
	strcpy(console_socket.path,connect.path);
	uint32_t remoteAddrLen = sizeof(remote);

	if ((console_socket.descriptor = accept(connect.descriptor,(struct sockaddr*)&remote,&remoteAddrLen)) == -1) {
		perror("accept");
		exit(1);
	}
	printf("Connected.\n");

	return console_socket;
}
