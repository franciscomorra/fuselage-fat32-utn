/*
 * tad_sockets.c
 *
 *  Created on: 22/09/2011
 *      Author: utn_so
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "tad_sockets.h"

socketInet_t SOCKET_inet_create(uint32_t style,char* address,uint32_t port,uint32_t mode)
{

	struct sockaddr_in sock_addr;
	memset(&sock_addr,0,sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	uint32_t sockfd = socket(sock_addr.sin_family, style, 0);

	if (sockfd < 0)
	{
		error("ERROR opening socket");

	}


	 if (mode == MODE_CONNECT)
	 {
		 struct hostent* server = gethostbyaddr(address,strlen(address),sock_addr.sin_family);

		 if (server == NULL)
		 {
			 fprintf(stderr,"ERROR, no such host\n");

		 }

		 sock_addr.sin_addr.s_addr = inet_addr(address);

	    if (connect(sockfd,(struct sockaddr *) &sock_addr,sizeof(sock_addr)) < 0)
	    {
	    	  error("ERROR connecting");

	    }

	 }
	 else if (mode == MODE_LISTEN)
	 {

		   sock_addr.sin_addr.s_addr = INADDR_ANY;
		   sock_addr.sin_port = htons(port);
		   if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0)
		   {
		       error("ERROR on binding");

		   }

		     listen(sockfd,5); //MAX_CONNECTIONS
		     /*clilen = sizeof(cli_addr);
		     newsockfd = accept(sockfd,
		                 (struct sockaddr *) &cli_addr,
		                 &clilen);
		     if (newsockfd < 0) */
	 }

	 	 	socketInet_t new_socket;
	 	    new_socket.descriptor = sockfd;
	 	    new_socket.port = port;
	 	    new_socket.address = (struct sockaddr*) &sock_addr;
	 	    return new_socket;
}

socketUnix_t SOCKET_unix_create(uint32_t style,char* path,uint32_t mode)
{
    struct sockaddr_un sock_addr;

	sock_addr.sun_family = AF_UNIX;
	strcpy(sock_addr.sun_path, path);
	unlink(sock_addr.sun_path);
	size_t len = strlen(sock_addr.sun_path) + sizeof(sock_addr.sun_family);

	uint32_t sockfd = socket(sock_addr.sun_family, style, 0);

	if (sockfd < 0)
	{
		error("ERROR opening socket");

	}


	 if (mode == MODE_CONNECT)
	 {
	    if (connect(sockfd,(struct sockaddr *) &sock_addr,len) < 0)
	    {
	    	  error("ERROR connecting");
	    }

	 }
	 else if (mode == MODE_LISTEN)
	 {

		   if (bind(sockfd, (struct sockaddr *) &sock_addr, len) < 0)
		   {
		       error("ERROR on binding");

		   }

		   if (listen(sockfd, 5) == -1) //MAX_CONNECTIONS
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
	 	    strcpy(new_socket.path,path);
	 	    new_socket.address = (struct sockaddr*) &sock_addr;
	 	    return new_socket;
}
