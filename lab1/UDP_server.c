#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "netw.h"
#define MAXLEN 256
#define MAX_PAYLOAD 512

int main(int argc, char *argv[])
{
	int opt;
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct addrinfo hints, *res;

	char buf[MAXLEN];
	char s_port[] = "1234";
	char payload[MAX_PAYLOAD] = "PAYLOAD:";
	const char optstr[] = "p:l:";
	while ((opt = getopt(argc, argv, optstr)) != -1)
	{
		switch (opt)
		{
			case 'l':
				strcpy(s_port, optarg);
				break;
			case 'p':
				strcat(payload, optarg);
				strcat(payload, "\n");
				break;
		}
	}
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfow(NULL, s_port, &hints, &res);

	int sockfd = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
	bindw(sockfd, res->ai_addr, res->ai_addrlen);

	while (1)
	{
		memset(buf, 0, MAXLEN);
		clilen = sizeof(cli_saddr);
		recvfromw(sockfd, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
		printf("primio %s\n`", buf);
		if (strncmp(buf, "HELLO", 5) == 0)
		{
			sendtow(sockfd, payload, strlen(payload), 0, (struct sockaddr *) &cli_saddr, sizeof(cli_saddr));
		};
	}
}