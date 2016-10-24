#include <stdio.h>
#include <sys/socket.h> 	/* Socket(), connect()*/
#include <string.h>		/*bzero() - should now use memset*/
#include <netinet/in.h>		/* struct sockaddr_in, htons()*/
#include <arpa/inet.h>		/* inet_aton() - should now use inet_pton()!*/
#include <unistd.h>		/* read() */
#include <netdb.h>		/* gethostbyname() */

#define MAXLINE 4096

int main(int argc, char* argv[])
{
	int sockfd;	/* the socket file descriptor */
	int nbytes;	/*number of bytes actually read from socket */
	char recvline[MAXLINE + 1]; /*To receive the day and time string */
	struct sockaddr_in servaddr; /* socket internet address structure*/
	struct hostent *servinfo; /* ptr to server /etc/hosts file entry*/
	
	if(argc != 2)
	{
		argv[1] = "localhost";
		printf("No IP supplied, using localhost\n");
		/*fprintf(stderr, "usage: %s <IPaddress>\n", argv[0]);*/
		/*return 1;*/
	}

	servinfo = gethostbyname(argv[1]); /* get ptr to the entry from /etc/hosts */
	if(!servinfo) /*if no /etc/hosts entry */
	{
		perror("gethostbyname error");
		return 1;
	}

	/*
	 * Create an internet (AF_INET) stream (SOCK_STREAM) socket
	 * with default protocol, i.e TCP socket.
	 * Try changing the default protocol from 0 to 1 to force error
	 */
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return 1;
	}

	/* zero the entire socket address structure to ensure we setup
	 * only what we want! then specify internet address family to AF_INET.
	 * Set the port to that of the daytime server (i.e. 13) converting it from
	 * host integer format to network format.
	 * Finallly, convert the ascii quad format of the IP addr to network format.
	 */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET; /* should now use PF_INET!*/
	servaddr.sin_port= htons(60000); /* daytime server "well known port"*/
	/* copy the host server address to the inet address structure */
	memcpy((char *)&servaddr.sin_addr, servinfo -> h_addr, servinfo -> h_length);	
	
	if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect error");
		return 1;
	}

	/*Read data from the network. No bytes read impllies end or error! */
	while( (nbytes = read(sockfd, recvline, MAXLINE)) > 0)
	{
		recvline[nbytes] = '\0'; /*end of string char*/
		if(fputs(recvline, stdout) == EOF)
		{
			perror("fputs error");
			return 1;
		}
	}

	if(nbytes < 0)
	{
		perror("read error");
		return 1;
	}

	exit(0);
}
