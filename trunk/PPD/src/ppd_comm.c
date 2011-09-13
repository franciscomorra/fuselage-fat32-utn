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
	char type; //TIPO DE MENSAJE
	memcpy(&type,msg,1);

	int payload_len = 0; //LARGO DEL PAYLOAD
	memcpy(&payload_len,msg+1,2);

	char payload[payload_len]; //PAYLOAD
	memcpy(payload,msg+2,payload_len);

	switch (type)
	{
		char* buf;
		int32_t fd;

		case HANDSHAKE:
		break;

		case WRITE:
		break;

		case READ:
			buf = (char*) malloc(512);
			fd = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
			leer_sector(fd,(int) payload[0],buf);
			return buf;
		break;
	}
}
