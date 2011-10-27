#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "ppdConsole_connect.h"

#define SOCK_PATH "/home/utn_so/CONSOLE_socket"


uint32_t CONNECT_toProcess(uint32_t* ppdFD){

	uint32_t len;
	struct sockaddr_un remote;
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, SOCK_PATH);

	if ((*ppdFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		 exit(1);
	}

	printf("Trying to connect...\n");

	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	while(connect(*ppdFD, (struct sockaddr *)&remote, len) == -1);

	printf("Connected.\n");
	return 0;
}
