/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_queue.h"
#include "praid_comm.h"

//A SER DEPRECADO, USAR COMMONS

uint32_t praid_READ_add(nipcMsg_t msgIn)
{return 0;}
uint32_t praid_WRITE_add(nipcMsg_t msgIn)
{return 0;}
uint32_t praid_READ_remove()
{return 0;}
uint32_t praid_WRITE_remove()
{return 0;}
uint32_t praid_READ_status()
{return 0;}
uint32_t praid_WRITE_status()
{return 0;}
read_node praid_READ_first()
{read_node read_node1;return read_node1;}
//Dame el primero
write_node praid_WRITE_first()
{write_node write_node1; return write_node1;}
