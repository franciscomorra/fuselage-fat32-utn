#include "praid_comm.h"
#include "ppd_misc.h"

uint32_t handle_WRITE_request()
{
	//TODO ver MUTEX semaforo, si la variable y el lock/unlock va a ir aca o en la funcion, no se bien el scope
	if(praid_WRITE_status()!=0){
		//TODO praid_WRITE_first();

	}
	handle_READ_request();
return 0;
}


uint32_t handle_READ_request()//
{return 0;}

//handle WRITE request Setea el semaforo, mira en la cola, hace lo tuyo con el nodo, libera la cola, send to PPD, handle READ
//handle READ request Setea el semaforo, mira en la cola, hace lo tuyo con el nodo, libera la cola
//TODO send to PFS mandale el NIPC, ver bien por el tema de sockets, habra que agregar un semaforo ahi?
//TODO send to PPD mandale el NIPC, espera a que te responda ver bien tema de sockets
//cambia tu propio estado READY, SINCRONIZE, WAIT_WRITE, WAIT_READ
