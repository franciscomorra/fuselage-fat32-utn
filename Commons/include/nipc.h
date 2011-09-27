#ifndef PRAID_REQUEST_H_
#define PRAID_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

typedef struct {
	char type;
	char len[2]; //la memoria es un gran array y aca reservo 2 posiciones
	char *payload;
} msgNIPC_t;

typedef struct {
	msgNIPC_t info;
	struct nipc_node* next;
}nipc_node;

msgNIPC_t* NIPC_createMsg(char type,uint32_t len, char* payload);

#endif /* PRAID_REQUEST_H_ */
