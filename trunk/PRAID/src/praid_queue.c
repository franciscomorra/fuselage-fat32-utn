/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_queue.h"
#include "tad_queue.h"

/*
Crear PRAID_LIST
Agregar nodo PRAID_LIST (Nuevo PPD)
Agregar nodo SUBLISTA Tipo Read (Usa un puntero que va recorriendo)
Agregar nodo SUBLISTA Tipo Write (A todos los nodos)
Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
Destruir nodo SUBLISTA
Buscar su nodo en PRAID_LIST, segun SocketPPD, o estado Synchronize
Sacar nodo SUBLISTA (para leerlo)

*/
