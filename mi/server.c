#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "netw.h"

#define MAXLEN 2048

typedef struct
{
	in_addr_t addr;
	in_port_t port;
}
msginfo;

void server(char *s_port, char *s_address, int n, int ttl)
{   
    struct sockaddr_in cli_saddr, fw_saddr;
    struct addrinfo hints, hints2, *res;
    msginfo *info;
    int sockfd, nbytes, fwsockfd;
    socklen_t clilen;
    char buf[MAXLEN];
    char message[MAXLEN - 6];
    char s_cliaddress[INET_ADDRSTRLEN];
    char s_cliport[20];
    char s_fwaddress[INET_ADDRSTRLEN];
    char s_fwport[20];
    printf("%d %d\n", ttl, n);
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if(strlen(s_address) > 0)
    {
	    getaddrinfow(s_address, s_port, &hints, &res);
    }
    else {
        getaddrinfow(NULL, s_port, &hints, &res);
    }

	sockfd = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
	bindw(sockfd, res->ai_addr, res->ai_addrlen);
    while(1){
        memset(buf, 0, MAXLEN);
        clilen = sizeof(cli_saddr);
		nbytes = recvfromw(sockfd, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
        clilen = sizeof(cli_saddr);
        info = (msginfo *) buf;
        inet_ntopw(AF_INET, (char *)&cli_saddr.sin_addr, s_cliaddress, INET_ADDRSTRLEN);
        memcpy(message, buf + 6, nbytes - 6);
        printf("Primljen datagram s %s:%d msg:%s\n", s_cliaddress, ntohs(cli_saddr.sin_port), message);
        
        memset(&hints2, 0, sizeof(hints2));
        hints2.ai_family = AF_INET;
        hints2.ai_socktype = SOCK_DGRAM;
        fw_saddr.sin_family = AF_INET;
        fw_saddr.sin_port = info->port;
        fw_saddr.sin_addr.s_addr = info->addr;
    	fwsockfd = socketw(fw_saddr.sin_family, SOCK_DGRAM, res->ai_protocol);
        inet_ntopw(AF_INET, (char *)&fw_saddr.sin_addr, s_fwaddress, INET_ADDRSTRLEN);
        printf("Saljem na: %s:%d\n", s_fwaddress,  ntohs(fw_saddr.sin_port));

        char *newmsg = calloc(nbytes, sizeof(char));
        memcpy(newmsg, &cli_saddr.sin_addr, sizeof(cli_saddr.sin_addr));
        memcpy(newmsg + sizeof(cli_saddr.sin_addr), &cli_saddr.sin_port, sizeof(cli_saddr.sin_port));
        memcpy(newmsg + 6, message, nbytes - 6);
        if(ttl != -1) {
            setsockoptw(fwsockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
        }
        for(int i = 0; i < n; i++) {
            sendtow(fwsockfd, newmsg, nbytes, 0, (struct sockaddr *) &fw_saddr, sizeof(fw_saddr)); 
        }
    }
}

int main(int argc, char *argv[])
{   
    int opt;
    
    const char optstr[] = "a:p:n:t:";
    char s_port[] = "1234";
    char s_address[INET_ADDRSTRLEN];
    int n = 1;
    int ttl = -1;
	while ((opt = getopt(argc, argv, optstr)) != -1)
	{
		switch (opt)
		{
			case 'a':
				strcpy(s_address, optarg);
				break;
			case 'p':
				strcpy(s_port, optarg);
				break;
            case 'n':
                n = strtol(optarg, NULL, 10);
                if ((errno == ERANGE && (n == LONG_MAX || n == LONG_MIN)) || (errno != 0 && n == 0)) {
                    perror("strtol n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                ttl = atoi(optarg);
                break;
		}
	}

    server(s_port, s_address, n, ttl);
}