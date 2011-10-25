/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_queue.h"
#include "tad_queue.h"


extern struct praid_list_node* PRAID_LIST;

praid_list_node* PRAID_list_appendNode(uint32_t tid)//Hay que pasarle el socket!
{
	queue_t *sublista = malloc(sizeof(queue_t));
	sublista->begin = NULL;
	sublista->end = sublista->begin;

	praid_list_node_content *data = malloc(sizeof(praid_list_node_content));
	data->tid = tid;
	data->ppdStatus = 1;//Sincronizando
	//SOCKET PPD!
	data->colaSublista = sublista;
	praid_list_node *aux = malloc(sizeof(praid_list_node));
	aux->info = data;
	aux->next = PRAID_LIST;
	PRAID_LIST = aux;
return aux;
}

/*
Agregar nodo SUBLISTA Tipo Read (Usa un puntero que va recorriendo)
Agregar nodo SUBLISTA Tipo Write (A todos los nodos)
Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
Destruir nodo SUBLISTA
Buscar su nodo en PRAID_LIST, segun SocketPPD, o estado Synchronize
Sacar nodo SUBLISTA (para leerlo)

*/
