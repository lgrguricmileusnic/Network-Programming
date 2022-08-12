#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "netw.h"

#define MAXLEN 256
#define MAX_PAYLOAD 512
#define BACKLOG 10
#define STDIN 0

void serve(char * tcp_port, char * udp_port, char * payload)
{
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct addrinfo hints, * res;
	int usockfd, tsockfd, fdmax, clifd;
	char buf[MAXLEN];
	int nbytes;
	fd_set master, readset;

	memset( & hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfow(NULL, udp_port, & hints, & res);

	usockfd = socketw(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
	bindw(usockfd, res -> ai_addr, res -> ai_addrlen);

	memset( & hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags |= AI_PASSIVE;

	getaddrinfow(NULL, tcp_port, & hints, & res);
	tsockfd = socketw(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
	bindw(tsockfd, res -> ai_addr, res -> ai_addrlen);
	listenw(tsockfd, BACKLOG);

	FD_ZERO( & master);
	FD_ZERO( & readset);
	FD_SET(STDIN, & master);
	FD_SET(usockfd, & master);
	FD_SET(tsockfd, & master);
	fdmax = tsockfd;
	while (1) {
		readset = master;
		selectw(fdmax + 1, &readset, NULL, NULL, NULL);
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &readset)) {
				if (i == tsockfd) {
					//handle new tcp client
					clilen = sizeof cli_saddr;
					clifd = acceptw(tsockfd, (struct sockaddr * ) &cli_saddr, &clilen);

					FD_SET(clifd, & master);
					if (clifd > fdmax)
						fdmax = clifd;
					printf("INCOMING TCP CONNECTION\n");
				}
				else if(i == usockfd) {
					//handle UDP request
					memset(buf, 0, MAXLEN);
					clilen = sizeof(cli_saddr);
					recvfromw(usockfd, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
					printf("primio %s\n", buf);
					if (strncmp(buf, "HELLO", 5) == 0)
					{
						sendtow(usockfd, payload, strlen(payload), 0, (struct sockaddr *) &cli_saddr, sizeof(cli_saddr));
					};
				}
				else if(i == STDIN) {
					//handle input from stdin
					memset(buf, 0, MAXLEN);
					read(i, buf, sizeof buf);
					if (strncmp(buf, "PRINT", 5) == 0) {
						printf("%s\n", payload);
					}
					else if (strncmp(buf, "SET", 3) == 0) {
						payload = malloc(MAX_PAYLOAD * sizeof(char));
						if (strlen(buf) <= 4){
							printf("usage: SET [payload]\n");
						}
						else {
							strcpy(payload, "PAYLOAD:");
							strcat(payload, buf + 4);
						}
					}
					else if (strncmp(buf, "QUIT", 4) == 0) {
						printf("Exiting with status 0.\n");
						exit(0);
					}
					else {
						printf("Unknown command.\n");
					}
				}
				else { 
					// handle data from a TCP client
					memset(buf, 0, MAXLEN);
					if ((nbytes = recvw(i, buf, sizeof buf, 0)) == 0) {
						// got error or connection closed by client
						printf("selectserver: socket %d hung up\n", i);
						close(i); 
						FD_CLR(i, & master);
					} else {
						printf("primio %s\n", buf);
						if (strncmp(buf, "HELLO", 5) == 0) {
							sendw(i, payload, strlen(payload), 0);
						};
					};
				};
			};
		};
	};

};

int main(int argc, char * argv[])
{
	int opt;
	char tcp_port[] = "1234";
	char udp_port[] = "1234";
	char payload[MAX_PAYLOAD] = "PAYLOAD:";
	const char optstr[] = "p:t:u:";

	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 't':
			strcpy(tcp_port, optarg);
			break;
		case 'u':
			strcpy(udp_port, optarg);
			break;
		case 'p':
			strcat(payload, optarg);
			strcat(payload, "\n");
			break;
		}
	}
	serve(tcp_port, udp_port, payload);
}

