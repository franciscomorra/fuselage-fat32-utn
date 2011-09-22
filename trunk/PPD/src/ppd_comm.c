// TEMPORAL

#include <fcntl.h>
#include <stdint.h>

#define HANDSHAKE 0
#define READ 2
#define WRITE 1

int32_t ppd_send(char* msg)
{

	return 1;
}

char* ppd_receive(char* msg)
{
	/* Se obtienen los distintos campos del mensaje IPC*/

	uint32_t type = 0;
	switch (type)
	{

		case HANDSHAKE:
		break;

		case WRITE:
		break;

		case READ:

		break;
	}
	return 0;
}
