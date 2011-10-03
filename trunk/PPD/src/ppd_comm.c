// TEMPORAL

#include <fcntl.h>
#include <stdint.h>
#include "nipc.h"


#define HANDSHAKE 0
#define READ 2
#define WRITE 1

extern requestNode_t first;

int32_t ppd_send(nipcMsg_t msg)
{

	return 1;
}

nipcMsg_t ppd_receive(nipcMsg_t msgIn )
{
	/* Se obtienen los distintos campos del mensaje IPC*/

	nipcMsg_t msgOut;
	uint32_t type = 0;
	switch (msgIn.type)
	{

		case HANDSHAKE:
		break;

		case WRITE:
		break;

		case READ:
			REQUEST_add(msgIn.payload,first);
		break;
	}
	return msgOut;
}
