/*
 * pfs_comm.c
 *
 *  Created on: 08/09/2011
 *      Author: utn_so
 */

#include <stdint.h>

#include "ppd_comm.h"
//ACA SE HACE LA CONEXION POR SOCKET Y LA VARIABLE QUE LA REPRESENTE SERA static
//PARA QUE SU SCOPE SEA SOLO DENTRO DE ESTE ARCHIVO QUE MANEJARA LAS CONEXIONES

int32_t pfs_send(char* msg)
{
	ppd_receive(msg);
}

char* pfs_receive(char* msg)
{

}
