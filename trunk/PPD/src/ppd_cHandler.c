
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> //ulink
#include "ppd_comm.h"
#include "nipc.h"
#include "ppd_common.h"
#include "ppd_cHandler.h"

#define SOCK_PATH "/home/utn_so/CONSOLE_socket"
#define LEN_MAX 512+sizeof(uint32_t)

extern uint32_t headPosition;

uint32_t CHANDLER_connect(uint32_t* consoleFD){
	uint32_t connectFD,remoteAddrLen,len;
	struct sockaddr_un local, remote;


	if ((connectFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(connectFD, (struct sockaddr *) &local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(connectFD, 10) == -1) {
		perror("listen");
		exit(1);
	}
		printf("Waiting for a connection...\n");

	remoteAddrLen = sizeof(remote);
	if ((*consoleFD = accept(connectFD, (struct sockaddr *) &remote,	&remoteAddrLen)) == -1) {
		perror("accept");
		exit(1);
	}
	printf("Connected.\n");
	return 0;
}

void CHANDLER_info(char* msg){
	// agrega la posicion actual del cabezal al mensaje
	requestNode_t* CHSPosition = malloc(sizeof(requestNode_t));
	COMMON_turnToCHS(headPosition,CHSPosition);
	memcpy(msg+3,&CHSPosition->cylinder,4);
	memcpy(msg+7,&CHSPosition->head,4);
	memcpy(msg+11,&CHSPosition->sector,4);
	free(CHSPosition);

}
