#include "praid_comm.h"
#include "ppd_misc.h"


//handle WRITE request Setea el semaforo, mira en la cola, hace lo tuyo con el nodo, libera la cola, send to PPD, handle READ
//handle READ request Setea el semaforo, mira en la cola, hace lo tuyo con el nodo, libera la cola
//send to PFS mandale el NIPC, ver bien por el tema de sockets, habra que agregar un semaforo ahi?
//send to PPD mandale el NIPC, espera a que te responda ver bien tema de sockets
//cambia tu propio estado READY, SINCRONIZE, WAIT_WRITE, WAIT_READ
//matate liberame bien la memoria
