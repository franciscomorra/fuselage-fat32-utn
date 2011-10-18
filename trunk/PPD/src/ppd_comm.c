// TEMPORAL

#include <fcntl.h>
#include <stdint.h>
#include "nipc.h"
#include "ppd_SSTF.h"
#include "ppd_comm.h"
#include "ppd_queue.h"

extern queue_t* queue;

uint32_t ppd_send(nipcMsg_t msg)
{

	return 1;
}

uint32_t ppd_receive(nipcMsg_t msgIn )
{
	/* Se obtienen los distintos campos del mensaje IPC*/

	uint32_t type = 0;
	if (msgIn.type == HANDSHAKE)
	{
	} else {
			QUEUE_add(msgIn,queue);
	}
	return 0;
}
