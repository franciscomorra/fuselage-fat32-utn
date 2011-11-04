#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
//#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nipc.h"
#include "ppd_SSTF.h"
#include "ppd_comm.h"
#include "tad_queue.h"
#include "ppd_common.h"
#include "ppd_translate.h"
#include "ppd_cHandler.h"
#include "ppd_qManager.h"

#define PORT 9034

//extern queue_t* queue;
extern multiQueue_t* multiQueue;
extern sem_t mainMutex;
//extern struct pollfd pollFds[2];
//extern sem_t queueElemSem;


uint32_t COMM_send(char* msg,uint32_t fd)
{
	switch(msg[0]){
		case PPDCONSOLE_INFO :{
		/*	pollFds[0].events = POLLOUT;
			poll(pollFds,2,-1);// avisame cuando pueda mandar datos
			if(pollFds[sockFD].revents & POLLOUT){
				if(send(pollFds[sockFD].fd,msg,15,NULL) == -1)
					perror("send");
			}
		*/
			send(fd,msg,15,0);
			break;
		}

		case PPDCONSOLE_TRACE :{
			uint16_t len;
			uint32_t sendLen;
			memcpy(&len,msg+1,2);
			sendLen = send(fd,msg,len+3,0);
			break;
		}

	}
	return 1;
}

uint32_t COMM_handleReceive(char* msgIn,uint32_t fd) {

	switch (msgIn[0]) {
		case HANDSHAKE:
				//TODO Handshake
			break;

		case PPDCONSOLE_INFO:{
			CHANDLER_info(msgIn);
			COMM_send(msgIn,fd);

			break;
		}

		default:{
			if(msgIn[0] == READ_SECTORS || msgIn[0] == WRITE_SECTORS || msgIn[0] == PPDCONSOLE_TRACE){

				request_t* request = TRANSLATE_fromCharToRequest(msgIn,fd);
				queue_t* queue = QMANAGER_selectPassiveQueue(multiQueue);

				sem_wait(&mainMutex);
				QUEUE_appendNode(queue,request);
				sem_post(&mainMutex);

				sem_post(&multiQueue->queueElemSem);


			}
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

uint32_t COMM_recieve(uint32_t currFD){
	char* msgHeader = malloc(3);							//alojo memoria para recibir la cabecera del mensaje (tipo y len)
	uint32_t recvReturn = 0;


	if((recvReturn = recv(currFD,msgHeader,3,0)) != 0)		//recibo la cabecera y la guardo en msgHeader
	{
		uint16_t len = 0;
		memcpy(&len,msgHeader+1,2);							//copio el len en un int para poder usarlo
		char* msgIn = malloc(len+3);						//alojo memoria para recibir el payload del mensaje
		recvReturn += recv(currFD,msgIn+3,len,0);			//recibo el mensaje y lo guardo en msgIn +3
		memcpy(msgIn,msgHeader,3);
		COMM_handleReceive(msgIn,currFD);
		free(msgIn);
	}

	free(msgHeader);
	return recvReturn;
}

