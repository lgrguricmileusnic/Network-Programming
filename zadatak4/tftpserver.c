#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include "netw.h"

#define MAXDATALEN 512
#define MAXLEN MAXDATALEN + 4
#define MAXNAMELEN 100 + 1
#define MAXERRMSGLEN 50 + 1
#define MAXTMODE 8 + 1 //max(len("netascii"), len("octet"))
#define CHUNKSIZE 512
#define NRETRIES 3
#define STIMEOUT 10
#define ASCII "netascii"
#define IDENT "lg52376:MrePro tftpserver"
#define LOPT LOG_PID 
#define LLEVEL LOG_INFO
#define LFACILITY LOG_FTP

typedef struct 
{
	uint16_t code;
	char rest[MAXLEN - 2];
} tftpmsg;

typedef struct 
{
	uint16_t code;
	uint16_t no;
} tftpack;

int d = 0;

size_t freadascii(char *buf, size_t len, FILE *f)
{
	int nbytes = fread(buf, 1, len, f);
	char out[MAXDATALEN * 2];
	
	int i = 0;
	int j = 0;

	while (i < nbytes)
	{
		if (buf[i] == '\n')
		{
			out[j++] = '\n';
			out[j] = '\r';
		}
		i++;j++;
	};

	fseek(f, i - j,ftell(f));
	buf = out;
	return nbytes;
}

void senddata(int sockfd, struct sockaddr_in *cli_saddr, int nbytes, char *buf, int blockno, socklen_t clilen)
{
	uint32_t code = 3;
	uint16_t ncode = htons(code);
	uint16_t nblockno = htons(blockno);
	char msg[MAXLEN];
	memcpy(msg, &ncode, sizeof(ncode));
	memcpy(msg + sizeof(ncode), &nblockno, sizeof(nblockno));
	memcpy(msg + 2 * sizeof(uint16_t), buf, nbytes);
	sendtow(sockfd, msg, 2 * sizeof(uint16_t) + nbytes, 0, (struct sockaddr*)cli_saddr, clilen);

}
void senderr(uint16_t errcode, int sockfd, struct sockaddr_in *cli_saddr, socklen_t clilen)
{
	uint16_t ncode = htons(5);
	uint16_t nerrcode = htons(errcode);
	char errmsg[MAXERRMSGLEN];
	char buf[MAXERRMSGLEN + 2 * sizeof(uint16_t)];
	switch (errcode)
	{
	case 0:
		strcpy(errmsg, "Request not supported");
		break;
	case 1:
		strcpy(errmsg, "File not found");
		break;
	case 2:
		strcpy(errmsg, "Access violation");
		break;
	case 4:
		strcpy(errmsg, "Illegal TFTP operation");
		break;
	default:
		break;
	}
	if(d) {
		syslog(LLEVEL, "TFTP ERROR %d %s\n", errcode, errmsg);
	} else {
		printf("TFTP ERROR %d %s\n", errcode, errmsg);
	}
	memcpy(buf, &ncode, sizeof(ncode));
	memcpy(buf + sizeof(ncode), &nerrcode, sizeof(nerrcode));
	strcpy(buf + 2 * sizeof(uint16_t), errmsg);
	sendtow(sockfd, buf, 2 * sizeof(uint16_t) + strlen(errmsg), 0, (struct sockaddr*)cli_saddr, clilen);
}

void serveRRQ(char *filename, char *tmode, struct sockaddr_in *cli_saddr, socklen_t clilen)
{
	FILE *f;
	struct timeval tv;
	int nrecvbytes, nbytes, blockno;
	char buf[CHUNKSIZE];
	char recvbuf[MAXLEN];
	tftpack *ack;

	int sockfd = socketw(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	tv.tv_sec = STIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if(access(filename, F_OK) < 0 || strchr(filename, '\\')) {
        senderr(0x01, sockfd, cli_saddr, clilen);
		exit(0);
    }
    else if(access(filename, R_OK) < 0) {
        senderr(0x02, sockfd, cli_saddr, clilen);
		exit(0);
    }
    else {
        f = fopen(filename, "rb");

        if (f == NULL) {
            senderr(0x01, sockfd, cli_saddr, clilen);
			exit(0);
        }
		blockno = 1;
		clilen = sizeof(*cli_saddr);
        while(1) {
			int end = 0;
			if (!strcmp(tmode, ASCII))
			{
				//ako je netascii
				nbytes = freadascii(buf, CHUNKSIZE, f);
			}
			else 
			{
				//ako je binary
				nbytes = fread(buf, 1, CHUNKSIZE, f);
			}
			for (int i = 0; i < NRETRIES + 1; i++)
			{
				senddata(sockfd, cli_saddr, nbytes, buf, blockno, clilen);
				nrecvbytes = recvfrom(sockfd, recvbuf, MAXLEN, 0, (struct sockaddr*)cli_saddr, &clilen);
				if (nrecvbytes != -1)
				{
					end = 0;
					break;
				}
				//printf("nrecvbytes %d\n", nrecvbytes);
				//printf("Retrying\n");
			}

			if (end) {
				//printf("Exceeded maximum number of retries. Transmission stop.\n");
				exit(0);
			}
			
			ack = (tftpack *)recvbuf;
			ack->code = ntohs(ack->code);
			ack->no = ntohs(ack->no);
			if (ack->code != 4)
			{
				//printf("Recieved code %d instead of ACK. Transmission stop.\n", ack->code);
				senderr(0x04, sockfd, cli_saddr, clilen);
				exit(0);
			}
			//printf("Recieved ACK for blockno %d\n", ack->no);
			if (ack->no != blockno)
			{
				//printf("Ack No. does not match block No.\n");
				senderr(0x04, sockfd, cli_saddr, clilen);
				exit(0);
			}
			
            if(nbytes < CHUNKSIZE)
                break;
			blockno++;
        }
        fclose(f);
    }
	//printf("RRQ handled\n");
	exit(0);
}

void serve(char *s_port)
{
	int nbytes;
	pid_t pid;
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct addrinfo hints, *res;
	char buf[MAXLEN];
	tftpmsg *msg;
	char filename[MAXNAMELEN];
	char tmode[MAXTMODE];
	char s_cliaddress[INET_ADDRSTRLEN];
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
		nbytes = recvfromw(sockfd, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
		msg = (tftpmsg*) buf;
		msg->code = ntohs(msg->code);
		memcpy(msg->rest, buf + 2, nbytes - 2);
		if(msg->code == 1)
		{
			//RRQ
			//printf("[MAIN] Primio RRQ\n");
			strcpy(filename, msg->rest);
			strcpy(tmode, msg->rest + strlen(filename) + 1);
			inet_ntopw(AF_INET, &(cli_saddr).sin_addr, s_cliaddress, INET_ADDRSTRLEN);
			if(d) {
				syslog(LLEVEL, "%s->%s\n", s_cliaddress, filename);
			} else {
				printf("%s->%s\n", s_cliaddress, filename);
			}
			//printf("tmode: %s\n", tmode);

			if((pid = fork()) == 0)
				serveRRQ(filename, tmode, &cli_saddr, clilen);
		}
		else
		{
			senderr(0x04, sockfd, &cli_saddr, clilen);
		}
	}
}

int main(int argc, char *argv[])
{
	int opt;
	char s_port[] = "1234";

	if (argc <= 1){
		printf("usage: tftpserver [-d] port_name_or_number\n");
		exit(1);
	}
	const char optstr[] = "d:";
	while ((opt = getopt(argc, argv, optstr)) != -1)
	{
		switch (opt)
		{
			case 'd':
				d = 1;
				daemon_init(IDENT, LFACILITY);
				break;
		}
	}
	if (optind < argc)
	{
		strcpy(s_port, argv[optind]);
	}
	serve(s_port);
}