// TEMPORAL

#include <fcntl.h>
#include <stdint.h>
#include "nipc.h"


#define HANDSHAKE 0
#define READ 2
#define WRITE 1

int32_t ppd_send(char* msg)
{

	return 1;
}

msgNIPC_t ppd_receive(msgNIPC_t msgIn)
{
	/* Se obtienen los distintos campos del mensaje IPC*/
	msgNIPC_t msgOut;
	uint32_t type = 0;
	switch (msgIn.type)
	{

		case HANDSHAKE:
		break;

		case WRITE:
		break;

		case READ:

		break;
	}
	return msgOut;
}
