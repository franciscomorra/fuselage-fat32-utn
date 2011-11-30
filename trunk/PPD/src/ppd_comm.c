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
#include "log.h"

#define PORT 9034

extern uint32_t Sector;
extern uint32_t Head;
extern uint32_t Cylinder;
extern multiQueue_t* multiQueue;
extern uint32_t HeadPosition;
extern t_log* Log;
extern sem_t queueMutex;
extern sem_t queueAvailableMutex;


uint32_t COMM_handleReceive(char* msgIn,uint32_t fd) {
	switch (msgIn[0]) {
/*		case HANDSHAKE:
		{
			uint16_t msgLen;
			memcpy(&msgLen,msgIn+1,2);
			if(msgLen == 0)
				COMM_send(msgIn,fd);
			else {
				uint16_t payloadLen = (strlen("El mensaje de Handshake es incorrecto")+1);
				char* errorMsg = malloc(payloadLen);
				strcpy(errorMsg,"El mensaje de Handshake es incorrecto");
				char* msgOut = malloc(3+payloadLen);
				NIPC_createCharMsg(msgOut,HANDSHAKE,payloadLen,errorMsg);
				COMM_send(msgOut,fd);
				log_error(Log,"Main","%s",errorMsg);
				free(errorMsg);
				free(msgOut);
				exit(1);
			}
			break;
		}
*/
		case PPDCONSOLE_INFO:{
			if(strcmp(msgIn+3,"JAMES")==0)
				HeadPosition = 7;

			CHS_t* CHSPosition = COMMON_turnToCHS(HeadPosition);

			memcpy(msgIn+3,&CHSPosition->cylinder,4);
			memcpy(msgIn+7,&CHSPosition->head,4);
			memcpy(msgIn+11,&CHSPosition->sector,4);
			COMM_send(msgIn,fd);

			free(CHSPosition);
			break;
		}
		case PPDCONSOLE_EXIT:{
			return 1;
		}

		default:{ // puede ser tanto de lectura, escritura o de tipo trace
			request_t* request = TRANSLATE_fromCharToRequest(msgIn,fd);
			if(*((uint32_t*) (msgIn+3)) != 0)
			{
				uint32_t breadk = 0;
			}
			char* msgType = COMMON_getTypeByFlag(request->type);

			sem_wait(&queueAvailableMutex);

			pthread_mutex_lock(&Log->mutex);
			sem_wait(&queueMutex);
			if(Log->log_levels == INFO){
				log_writeHeaderWithoutMutex(Log,"Principal",Log->log_levels);
				fprintf(Log->file,"Ingreso de pedido de sector: (%d:%d:%d) de tipo: %s\n",request->CHS->cylinder,request->CHS->head,request->CHS->sector,msgType);
			}
			sem_post(&multiQueue->queueElemSem);
			queue_t* queue = QMANAGER_selectPassiveQueue(multiQueue);
			QUEUE_appendNode(queue,request);
			sem_post(&queueMutex);
			pthread_mutex_unlock(&Log->mutex);


			free(msgType);


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
	console_socket.path = malloc(strlen(connect.path)+1);
	strncpy(console_socket.path,connect.path,strlen(connect.path)+1);
	uint32_t remoteAddrLen = sizeof(remote);

	if ((console_socket.descriptor = accept(connect.descriptor,(struct sockaddr*)&remote,&remoteAddrLen)) == -1) {
		perror("accept");
		exit(1);
	}
	printf("Connected.\n");

	return console_socket;
}

void COMM_RaidHandshake(socketInet_t inetListen,uint32_t diskID){
	uint16_t handshakeLen = sizeof(uint32_t)*2;
	char* handshakePayload = malloc(handshakeLen);
	char* handshake = malloc(3 + handshakeLen);
	uint32_t totalSectors = Cylinder * Sector * Head;

	memcpy(handshakePayload,&diskID,4);
	memcpy(handshakePayload+4,&totalSectors,4);
	NIPC_createCharMsg(handshake,HANDSHAKE,handshakeLen,handshakePayload);

	COMM_send(handshake,inetListen.descriptor);
 	free(handshakePayload);
	free(handshake);

	uint32_t dataReceived = 0;
	uint32_t len;
	char* msgIn = COMM_receiveAll(inetListen.descriptor,&dataReceived,&len);
	if(dataReceived > 3){
		uint16_t len;
		memcpy(&len,msgIn,2);
		char* errorMsg = malloc(len);
		memcpy(&errorMsg,msgIn+3,len);
		log_error(Log,"Main","%s",errorMsg);
		free(msgIn);
		free(errorMsg);
		exit(1);
	}

}
