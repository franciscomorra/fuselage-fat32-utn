#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ppdConsole_connect.h"

uint32_t sockCHandler;

void CONNECT_toProcess(struct sockaddr_un remote){

	uint32_t t;
	uint32_t len;

    if ((sockCHandler = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");


    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(sockCHandler, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");
}
