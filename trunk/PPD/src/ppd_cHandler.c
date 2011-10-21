
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ppd_comm.h"
#include "nipc.h"
#include "ppd_common.h"
#include "ppd_cHandler.h"

extern uint32_t headPosition;

#define SOCK_PATH "CONSOLE_socket"
#define LEN_MAX 512+sizeof(uint32_t)

void CHANDLER_manager(){
	uint32_t sockConnect, sockConsole, remoteAddrLen, len;
	struct sockaddr_un local, remote;
	nipcMsg_t* msgIn;

	if ((sockConnect = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(sockConnect, (struct sockaddr *) &local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockConnect, 5) == -1) {
		perror("listen");
		exit(1);
	}

	while (1) {
		uint32_t done, n;
		printf("Waiting for a connection...\n");
		remoteAddrLen = sizeof(remote);
		if ((sockConsole = accept(sockConnect, (struct sockaddr *) &remote,
				&remoteAddrLen)) == -1) {
			perror("accept");
			exit(1);
		}

		printf("Connected.\n");

		done = 0;
		do {
			n = recv(sockConsole, msgIn, LEN_MAX, 0);
			if (n <= 0) {
				if (n < 0)
					perror("recv");
				done = 1;
			} else {
				ppd_receive(*msgIn);
			}
			/*
			 if (!done)
			 if (send(sockConsole, str, n, 0) < 0) {
			 perror("send");
			 done = 1;
			 }	*/
		} while (!done);

		close(sockConsole);
	}
}


void CHANDLER_info(){
	requestNode_t* CHSPosition = malloc(sizeof(requestNode_t));
	COMMON_turnToCHS(headPosition,CHSPosition);
	printf("La posicion actual del cabezal es: (%d,%d,%d).\n",CHSPosition->cylinder,CHSPosition->head,CHSPosition->sector);
	free(CHSPosition);
}
