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

	queue_t *sublista;
	QUEUE_initialize(&sublista);

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
//TODO Creacion Subnodo Sincronizacion
	ppd_sublist_node_content *contenido_nodo_sublista= malloc(sizeof(ppd_sublist_node_content));
	char* sectorCount = 0;//Primer Sector del disco
	//contenido_nodo_sublista->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),sectorCount);
	contenido_nodo_sublista->synch = 1;
	//SOCKETS!

	queueNode_t *nodo_sublista = QUEUE_createNode(contenido_nodo_sublista);
	pthread_mutex_lock(&mutex_LIST);
	TODO Ver como se usan las funciones del queue bien
	//QUEUE_initialize(&self_list_node->info->colaSublista);
	//QUEUE_appendNode(&self_list_node->info->colaSublista, nodo_sublista);

	pthread_mutex_unlock(&mutex_LIST);
*/



/*
Agregar nodo SUBLISTA Tipo Read (Usa un puntero que va recorriendo)
Agregar nodo SUBLISTA Tipo Write (A todos los nodos)
Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
Destruir nodo SUBLISTA
Sacar nodo SUBLISTA (para leerlo)

*/
