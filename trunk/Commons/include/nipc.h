#ifndef PRAID_REQUEST_H_
#define PRAID_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

typedef struct {
	char type;
	uint32_t len;
	uint32_t* payload;
} msgNIPC_t;

typedef struct {
	msgNIPC_t info;
	struct nipc_node* next;
}nipc_node;

#endif /* PRAID_REQUEST_H_ */
