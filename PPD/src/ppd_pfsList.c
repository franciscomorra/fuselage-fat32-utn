#include <stdint.h>
#include <semaphore.h>

#include "ppd_pfsList.h"
#include "tad_queue.h"

pfs_node_t* PFSLIST_getByFd(queue_t pfsList,uint32_t fd){
	queueNode_t* cur_pfs_node = pfsList.begin;

	while(cur_pfs_node != NULL){
		pfs_node_t* cur_pfs = cur_pfs_node->data;
		if(cur_pfs->sock_fd == fd)
			return cur_pfs;
		cur_pfs_node = cur_pfs_node->next;
	}
	exit(1);
}

void PFSLIST_addNew(queue_t* pfsList,uint32_t fd){
	pfs_node_t* new_pfs = malloc(sizeof(pfs_node_t));

	new_pfs->sock_fd = fd;
pthread_mutex_init(&new_pfs->sock_mutex,NULL);
	QUEUE_appendNode(pfsList,new_pfs);
}
