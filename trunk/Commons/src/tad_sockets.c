/*
 * tad_sockets.c
 *
 *  Created on: 22/09/2011
 *      Author: utn_so
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/errno.h>

#include "tad_sockets.h"

socketInet_t SOCKET_inet_create(uint32_t style,char* address,uint32_t port,uint32_t mode)
{

	socketInet_t new_socket;

	struct sockaddr_in sock_addr;
	memset(&sock_addr,0,sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	int32_t sockfd = socket(sock_addr.sin_family, style, 0);

	int32_t optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	if (sockfd < 0)
	{
		new_socket.status = errno;
		return new_socket;
	}


	 if (mode == MODE_CONNECT)
	 {

		 sock_addr.sin_addr.s_addr = inet_addr(address);

	    if (connect(sockfd,(struct sockaddr *) &sock_addr,sizeof(sock_addr)) < 0)
	    {
	    	new_socket.status = errno;
	    	return new_socket;
	    }

	 }
	 else if (mode == MODE_LISTEN)
	 {

		   sock_addr.sin_addr.s_addr = INADDR_ANY;
		   sock_addr.sin_port = htons(port);
		   if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0)
		   {
			   new_socket.status = errno;
			   return new_socket;

		   }

		     if (listen(sockfd,5) == -1)
		     {
		    	 new_socket.status = errno;
		    	 return new_socket;
		     }
		     /*clilen = sizeof(cli_addr);
		     newsockfd = accept(sockfd,
		                 (struct sockaddr *) &cli_addr,
		                 &clilen);
		     if (newsockfd < 0) */
	 }


	 	 	new_socket.style = style;
	 	 	new_socket.mode = mode;
	 	    new_socket.descriptor = sockfd;
	 	    new_socket.status = SOCK_OK;
	 	    new_socket.port = port;
	 	    new_socket.address = (struct sockaddr*) &sock_addr;
	 	    return new_socket;
}

socketUnix_t SOCKET_unix_create(uint32_t style,char* path,uint32_t mode)
{
    struct sockaddr_un sock_addr;

	sock_addr.sun_family = AF_UNIX;
	strcpy(sock_addr.sun_path, path);
	size_t len = strlen(sock_addr.sun_path) + sizeof(sock_addr.sun_family);

	int32_t sockfd = socket(sock_addr.sun_family, style, 0);

	if (sockfd < 0)
	{
		error("ERROR opening socket");

	}

	if(mode == MODE_CONNECT)
		while(connect(sockfd,(struct sockaddr*) &sock_addr,len)==-1);

	 else if (mode == MODE_LISTEN)
	 {
		 unlink(sock_addr.sun_path);

		 if (bind(sockfd, (struct sockaddr *) &sock_addr, len) < 0)
		 {
			 error("ERROR on binding");

		 }

		 if (listen(sockfd,1) == -1) //MAX_CONNECTIONS
		 {
		     perror("listen");
		 }
		    /*clilen = sizeof(cli_addr);
		     newsockfd = accept(sockfd,
		                 (struct sockaddr *) &cli_addr,
		                 &clilen);
		     if (newsockfd < 0) */
	 }

	 	 	socketUnix_t new_socket;
	 	    new_socket.descriptor = sockfd;
	 	    new_socket.style = style;
	 	    new_socket.path = malloc(strlen(path)+1);
	 	    strncpy(new_socket.path,path,strlen(path));
	 	    new_socket.address = (struct sockaddr*) &sock_addr;
	 	    return new_socket;
}

int32_t SOCKET_sendAll(uint32_t fd, char *buf, uint32_t len,uint32_t opt)
{
	int32_t total = 0;
	int32_t left = len;
	int32_t sent = 0;

	while (total < len)
	{
		sent = send(fd,buf+total,left,opt);
		if (sent <= 0) {break;}
		total += sent;
		left -= sent;
	}

	if (sent == 0)
		return SOCK_DISCONNECTED;
	else if (sent == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return SOCK_NODATA;
		else
			return SOCK_ERROR;
	}

	return total;
}

int32_t SOCKET_recvAll(uint32_t fd, char *buf, uint32_t len,uint32_t opt)
{
	int32_t total = 0;
	int32_t left = len;
	int32_t received = 0;

	while (total < len)
	{
		received = recv(fd,buf+total,left,opt);
		if (received <= 0) {break;}
		total += received;
		left -= received;
	}



	if (received == 0)
		return SOCK_DISCONNECTED;
	else if (received == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return SOCK_NODATA;
		else
			return SOCK_ERROR;
	}

	return total;
}

uint32_t SOCKET_canSend(uint32_t fd)
{
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=0;

	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(fd,&write_set);

	return select(fd+1,NULL,&write_set,NULL,&timeout) <= 0 ? 0 : 1;
}
