#ifndef NIPC_H_
#define NIPC_H_

#include <stdio.h>
#include <stdint.h>

typedef enum {
	HANDSHAKE=0x00, READ_SECTORS=0x01, WRITE_SECTORS=0x02, PPDCONSOLE_INFO=0x03, PPDCONSOLE_TRACE=0x04
} NIPC_type;

typedef struct NIPC_msg {
	NIPC_type type;
	char len[2];
	char *payload;
} __attribute__((__packed__)) nipcMsg_t;


typedef struct {
	nipcMsg_t msg;
	struct NIPC_node* next;
} NIPC_node;

nipcMsg_t NIPC_createMsg(NIPC_type type,uint32_t len, char* payload);

void NIPC_cleanMsg(nipcMsg_t* msg);


nipcMsg_t NIPC_toMsg(char* msg);

char* NIPC_toBytes(nipcMsg_t msg);


char* NIPC_createCharMsg(NIPC_type type,uint32_t payload_bytes_len,char* payload_bytes);


#endif /* NIPC_H_ */
