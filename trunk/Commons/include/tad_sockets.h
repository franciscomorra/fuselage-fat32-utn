/*
 * tad_sockets.h
 *
 *  Created on: 22/09/2011
 *      Author: utn_so
 */

#ifndef TAD_SOCKETS_H_
#define TAD_SOCKETS_H_

#define MODE_LISTEN 0
#define MODE_CONNECT 1

typedef struct socketInet_t
{
	uint32_t descriptor;
	uint32_t style;
	uint32_t mode;
	struct sockaddr * address;
	uint32_t port;




} socketInet_t;

typedef struct socketUnix_t
{
	uint32_t descriptor;
	uint32_t style;
	uint32_t mode;
	struct sockaddr * address;
	char* path;

} socketUnix_t;

#endif /* TAD_SOCKETS_H_ */
