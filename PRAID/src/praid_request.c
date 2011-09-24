#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include "nipc.h"


uint32_t addRequest(msgNIPC_t msg,nipc_node* first, nipc_node* last){
	nipc_node* new = malloc(sizeof(nipc_node));
	new->info = msg;
	new->next = 0x0;

	if(first == 0x0){
		first = new;
		last = new;
	}
	else {
		last->next = new;
		last = new;
	}

	return 0;
}

uint32_t takeRequest(msgNIPC_t msg, nipc_node* first) {
	if (first == 0x0)
			return 1;
	nipc_node* aux;
	first = aux;
	msg = first->info;
	first = first->next;
	free(aux->info);
	free(aux->next);
	free(aux);
	return 0;
}

