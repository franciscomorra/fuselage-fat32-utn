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
#include <errno.h>
#include"sockets.h"
#define PORT 9333

uint32_t Create_Sockets_INET(uint32_t* listenFD){

	struct sockaddr_in dir;

	if((*listenFD = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	dir.sin_family = AF_INET;
	dir.sin_addr.s_addr = INADDR_ANY;
	dir.sin_port = htons(PORT);
	memset(&(dir.sin_zero),'\0',8);

	if(bind(*listenFD,(struct sockaddr *)&dir,sizeof(dir))==-1){
		perror("bind");
		exit(1);
	}
	if(listen(*listenFD,10) == -1){
		perror("listen");
		exit(1);
	}

	return 0;
}

/* Se obtienen los distintos campos del mensaje IPC*/
uint32_t ppd_receive(char* msgIn,uint32_t fd) {



	if(msgIn[0]== HANDSHAKE){

				//TODO Handshake
	}

	if(msgIn[0] == READ_SECTORS || msgIn[0] == WRITE_SECTORS){

		//TODO LISTA
	}
}
