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
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	praid_list_node_content *data = malloc(sizeof(praid_list_node_content));
	queue_t* subList = malloc(sizeof(queue_t));
	queueNode_t *subListNode = malloc(sizeof(queueNode_t));
	praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));

	data->tid = tid;
	data->ppdStatus = 1;//Sincronizando
	//SOCKET PPD!

	data_sublist->synch = 1;
	//SOCKET PPD
	//NIPC Type WRITE_SECTORS
	QUEUE_initialize(subList);

	subListNode->data = (praid_sl_content*)data_sublist;

	subListNode->next = NULL;
	subList->begin = subList->end = subListNode;

//	QUEUE_appendNode(subList, data_sublist);
/*
	praid_sl_content data_SL_memcpy;
	memcpy(&data_SL_memcpy,subListNode->data,sizeof(praid_sl_content));
*/
	data->colaSublista = subList;
	nodoLISTA->info = data;
	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;

return nodoLISTA;
}





/*
Agregar nodo SUBLISTA Tipo Read (Usa un puntero que va recorriendo)
Agregar nodo SUBLISTA Tipo Write (A todos los nodos)
Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
Destruir nodo SUBLISTA
Sacar nodo SUBLISTA (para leerlo)

*/
