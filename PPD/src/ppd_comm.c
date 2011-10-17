// TEMPORAL

#include <fcntl.h>
#include <stdint.h>
#include "nipc.h"
#include "ppd_SSTF.h"
#include "ppd_comm.h"

int32_t ppd_send(nipcMsg_t msg)
{

	return 1;
}

nipcMsg_t ppd_receive(nipcMsg_t msgIn )
{
	/* Se obtienen los distintos campos del mensaje IPC*/

	uint32_t type = 0;
	if (msgIn.type == HANDSHAKE)
	{
	} else {
			SSTF_addRequest((uint32_t*)msgIn.payload); //casteamos el puntero para leer el numero de sector
	}
	return 0;
}
