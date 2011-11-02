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

#define PORT 9034

extern queue_t* queue;
extern sem_t mainMutex;
//extern struct pollfd pollFds[2];
extern sem_t queueElemSem;


uint32_t ppd_send(char* msg,uint32_t fd)
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
			send(fd,msg,15,NULL);
			break;
		}

		case PPDCONSOLE_TRACE :{
			uint32_t len;
			memcpy(&len,msg+1,2);
			send(fd,msg,len+3,NULL);
			break;
		}

	}
	return 1;
}

uint32_t ppd_receive(char* msgIn,uint32_t fd) {
	switch (msgIn[0]) {
		case HANDSHAKE:
				//TODO Handshake
			break;

		case PPDCONSOLE_INFO:{
			CHANDLER_info(msgIn);
			ppd_send(msgIn,fd);

			break;
		}

		default:{
			if(msgIn[0] == READ_SECTORS || msgIn[0] == WRITE_SECTORS || msgIn[0] == PPDCONSOLE_TRACE){

				requestNode_t* request;
				queueNode_t* queueNode;

				request = TRANSLATE_fromCharToRequest(msgIn,fd);

				sem_wait(&mainMutex);
				QUEUE_appendNode(queue,request);
				sem_post(&mainMutex);
				sem_post(&queueElemSem);


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

