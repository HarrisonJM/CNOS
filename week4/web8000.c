/* web8000.c a simple web server*/

#define WEBROOT "/home/harry/CNOS/week4"
#define HTTP_PORT 8000

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

int create_tcp_endpoint(int port); /*prototype for fn to open socket*/

void text (int fd, char * s) /*simple fn to write to file */
{
	write(fd, s, strlen(s));
}

int main (void)
{
	int fdnet, fd, count, sock;
	char filename[150], request[1024], reply[1024];

	/*create an endpoint to listen on*/
	if((sock = create_tcp_endpoint(HTTP_PORT)) < 0)
	{
		fprintf(stderr, "cannot create endpoint\n");
		exit(1);
	}

	/*enter the main service loop*/
	while(1)
	{
		/* Get a connection from a client*/
		fdnet = accept(sock, NULL, NULL);
		fprintf(stderr, "connected on fd %d\n", fdnet);

		/*READ THE GET REQUEST & BUILD THE FILE NAME */
		/*i.e. somethring like request[] = "GET /file.html HTTP/1.1" */
		read(fdnet, request, sizeof(request));

		strtok(request, " ");
		strcpy(filename, WEBROOT);
		strcat(filename, strtok(NULL, " "));
		printf("Requested file is %s\n", filename);
		fd = open(filename, O_RDONLY);

		if(fd < 0) /* check to see if the file is there */
		{
			text(fdnet, "HTTP/1.1 404 NOT FOUND\n");
			text(fdnet, "Content-Type: text/html\n\n");
			text(fdnet, "<HTML><BODY> NOT FOUND </BODY></HTML>");
		}
		else
		{
			text(fdnet, "HTTP/1.1 200 OK\n");
			text(fdnet, "Content-Type: text/html\n\n");
			while((count = read(fd, reply, 1024)) > 0)
			{
				write(fdnet, reply, count);
			}
		}
		
		close(fd);
		close(fdnet);

		printf("REQUEST: %s", text);
	}/* while(1) */
}/* main */

/* Useful function to create server endpoint */
int create_tcp_endpoint(int port)
{
	int sock;
	struct sockaddr_in server;
	/*make socket with TCP streams. Kernel choose a suitable protocol*/
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0) /* socket creation failed*/
	{
		return -1;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY); /* any server's IP addr*/
	server.sin_port = htons(port);

	if( bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		return -2; /*bind fail*/
	}

	listen(sock, 5); /*a listening socket with a max queueu of 5 clients */
	return sock;
}
