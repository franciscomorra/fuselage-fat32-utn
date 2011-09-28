#ifndef PRAID_REQUEST_H_
#define PRAID_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

typedef enum {
	HANDSHAKE=0x00, READ_SECTORS=0x01, WRITE_SECTORS=0x02
} NIPC_type;

typedef struct NIPC_msg {
	NIPC_type type;
	char len[2]; //la memoria es un gran array y aca reservo 2 posiciones
	char *payload;
} __attribute__((__packed__)) NIPC_msg;

typedef struct {
	NIPC_msg msg;
	struct nipc_node* next;
} NIPC_node;

NIPC_msg NIPC_createMsg(NIPC_type type,uint32_t len, char* payload);

#endif /* PRAID_REQUEST_H_ */
