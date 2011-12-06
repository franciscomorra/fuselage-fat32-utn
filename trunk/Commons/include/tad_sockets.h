/*
 * tad_sockets.h
 *
 *  Created on: 22/09/2011
 *      Author: utn_so
 */

#ifndef TAD_SOCKETS_H_
#define TAD_SOCKETS_H_

#include <stdint.h>
#include <pthread.h>
#define MODE_LISTEN 0
#define MODE_CONNECT 1
#define SOCK_OK 0
#define SOCK_ENOHOST 1
#define SOCK_FREE 2
#define SOCK_NOTFREE 3

#define SOCK_DISCONNECTED -2
#define SOCK_NODATA -3
#define SOCK_ERROR -4

typedef struct socketInet_t
{
	uint32_t descriptor;
	uint32_t style;
	uint32_t mode;
	uint32_t status;
	struct sockaddr * address;
	uint32_t port;
	pthread_mutex_t sock_mutex;
} socketInet_t;

typedef struct socketUnix_t
{
	uint32_t descriptor;
	uint32_t style;
	uint32_t mode;
	uint32_t status;
	struct sockaddr * address;
	char* path;


} socketUnix_t;


socketInet_t SOCKET_inet_create(uint32_t style,char* address,uint32_t port,uint32_t mode);
socketUnix_t SOCKET_unix_create(uint32_t style,char* path,uint32_t mode);
int32_t SOCKET_sendAll(uint32_t fd, char *buf, uint32_t len,uint32_t opt);
int32_t SOCKET_recvAll(uint32_t fd, char *buf, uint32_t len,uint32_t opt);
uint32_t SOCKET_canSend(uint32_t fd);

#endif /* TAD_SOCKETS_H_ */
