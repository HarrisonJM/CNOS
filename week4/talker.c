/* talker.c - a datagram 'client'
 * ned to supply host name/IPO and one word message,
 * e.g. talker localhost hello
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> /* for gethostbyname() */

#define PORT 4950

int main(int argc, char * argv[])
{
	int sockfd, numbytes, addr_len;
	struct hostent *he;
	struct sockaddr_in their_addr; /* server address info */

	if(argc != 3)
	{
		fprintf(stderr, "usage: talker hostname message\n");
		exit(1);
	}
	
	/* resolve server host name or IP address */
	if((he = gethostbyname(argv[1])) == NULL)
	{
		perror("Talker gethostbyname");
		exit(1);
	}

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Talker socket");
		exit(1);
	}

	memset(&their_addr, 0, sizeof(their_addr)); 	/* zero struct*/
	their_addr.sin_family = AF_INET;		/*...host byte order*/
	their_addr.sin_port = htons(PORT);	/*...short, netwk byte order*/
	their_addr.sin_addr = *((struct in_addr *)he -> h_addr);


	if((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
		(struct sockaddr*)&their_addr, sizeof(struct sockaddr))) == -1)
	{
		perror("Talker sendto");
		exit(1);
	}

	printf("Sent %d bytes to %s\n", numbytes,
					inet_ntoa(their_addr.sin_addr));

	addr_len = sizeof(struct sockaddr);
	
	/* receives achoed content*/
	if((numbytes = recvfrom(sockfd, argv[2], strlen(argv[2]), 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1)
	{
		perror("recvfrom fail");
		exit(1);
	}

	printf("Echo: %s\n", argv[2]);
		

	close(sockfd);
	return 0;
}













