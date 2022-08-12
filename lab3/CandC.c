#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdarg.h>
#include "netw.h"

#define MAXLEN 65536
#define MAX_PAYLOAD 512
#define MAX_NBOTS 10
#define BACKLOG 10
#define STDIN 0

typedef struct
{
	char addr[INET_ADDRSTRLEN];
	char port[22];
}
ippair;

typedef struct
{
	char command;
	ippair ippairs[20];
}
msg;

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },  {"jpg", "image/jpg" }, {"jpeg","image/jpeg"},
	{"png", "image/png" },  {"ico", "image/ico" },  {"zip", "image/zip" },
	{"gz",  "image/gz"  },  {"tar", "image/tar" },  {"htm", "text/html" },
	{"html","text/html" },  {"pdf", "application/pfd"}, {"txt", "text/plain"}, {0,0}};

const char help[] = 
				"pt... bot klijentima šalje poruku PROG_TCP (struct MSG:1 10.0.0.20 1234\\n)\n"
                "ptl.. bot klijentima šalje poruku PROG_TCP (struct MSG:1 127.0.0.1 1234\\n)\n"
                "pu... bot klijentima šalje poruku PROG_UDP (struct MSG:2 10.0.0.20 1234\\n)\n"
                "pul.. bot klijentima šalje poruku PROG_UDP (struct MSG:2 127.0.0.1 1234\\n)\n"
                "r.... bot klijentima šalje poruku RUN s adresama lokalnog računala:\n"
                "	   struct MSG:3 127.0.0.1 vat localhost 6789\\n\n"
                "r2... bot klijentima šalje poruku RUN s adresama računala iz IMUNES-a:\n"
                "	   struct MSG:3 20.0.0.11 1111 20.0.0.12 2222 20.0.0.13 hostmon\n"
                "s.... bot klijentima šalje poruku STOP (struct MSG:4)\n"
                "l.... lokalni ispis adresa bot klijenata\n"
                "n.... šalje poruku: NEPOZNATA\\n\n"
                "q.... bot klijentima šalje poruku QUIT i završava s radom (struct MSG:0)\n"
                "h.... ispis naredbi\n";

struct sockaddr_in bots[MAX_NBOTS];
socklen_t botlens [MAX_NBOTS];
int nbots;
socklen_t usockfd;


void sendto_all(char *buf, int nbytes)
{
    for (int i = 0; i < nbots; i++)
    {
        sendtow(usockfd, buf, nbytes, 0, (struct sockaddr *) &bots[i], botlens[i]);
    }
}

void sendmsgto_all(msg m, int nippairs)
{
	char buf[MAXLEN];
	int bufsize;
	memcpy(buf, &m.command, sizeof(m.command));
	memcpy(buf + sizeof(m.command), m.ippairs, nippairs * sizeof(ippair));
	bufsize = sizeof(m.command) + nippairs * sizeof(ippair);
	sendto_all(buf, bufsize);
}



void handle_cmd(char *buf)
{
	msg m;
	if (strncmp(buf, "ptl", 3) == 0) {
		ippair server = {"127.0.0.1", "1234"};
		m.command = '1';
		memcpy(m.ippairs, &server, sizeof(ippair));
		sendmsgto_all(m, 1);
    }
    else if (strncmp(buf, "pt", 2) == 0) {
		ippair server = {"10.0.0.20", "1234"};
		m.command = '1';
		memcpy(m.ippairs, &server, sizeof(ippair));
		sendmsgto_all(m, 1);
    }
	else if (strncmp(buf, "pul", 3) == 0) {
		ippair server = {"127.0.0.1", "1234"};
		m.command = '2';
		memcpy(m.ippairs, &server, sizeof(ippair));
		sendmsgto_all(m, 1);
    }
    else if (strncmp(buf, "pu", 2) == 0) {
		ippair server = {"10.0.0.20", "1234"};
		m.command = '2';
		memcpy(m.ippairs, &server, sizeof(ippair));
		sendmsgto_all(m, 1);
    }
	else if (strncmp(buf, "r2", 2) == 0) {
		//struct MSG:3 20.0.0.11 1111 20.0.0.12 2222 20.0.0.13 hostmon\n
		ippair addr1, addr2, addr3;
		addr1 = (ippair) {"20.0.0.11", "1111"};
		addr2 = (ippair) {"20.0.0.12", "2222"};
		addr3 = (ippair) {"20.0.0.13", "hostmon"};
		m.command = '3';
		memcpy(m.ippairs, &addr1, sizeof(ippair));
		strcpy(m.ippairs[1].addr, addr2.addr);
		strcpy(m.ippairs[1].port, addr2.port);
		strcpy(m.ippairs[2].addr, addr3.addr);
		strcpy(m.ippairs[2].port, addr3.port);
		sendmsgto_all(m, 3);
    }
    else if (strncmp(buf, "r", 1) == 0) {
		ippair addr1, addr2;
		addr1 = (ippair) {"127.0.0.1", "3456"};
		addr2 = (ippair) {"localhost", "6789"};
		m.command = '3';
		memcpy(m.ippairs, &addr1, sizeof(ippair));
		strcpy(m.ippairs[1].addr, addr2.addr);
		strcpy(m.ippairs[1].port, addr2.port);
		sendmsgto_all(m, 2);
    }

    else if (strncmp(buf, "s", 1) == 0) {
		m.command = '4';
		sendmsgto_all(m, 4);
    }
    else if (strncmp(buf, "l", 1) == 0) {
		char addr[INET_ADDRSTRLEN];
		for (int i = 0; i < nbots; i++)
		{
			inet_ntopw(AF_INET, &(bots[i]).sin_addr, addr, INET_ADDRSTRLEN);
			printf("%s\n",addr);
		}
    }
    else if (strncmp(buf, "n", 1) == 0) {
		char buf[] = "NEPOZNATA\n";
		sendto_all(buf, strlen(buf));
    }
    else if (strncmp(buf, "q", 1) == 0) {
		m.command = '0';
		sendmsgto_all(m,0);
        printf("Quitting\n");
        exit(0);
    }
    else if (strncmp(buf, "h", 1) == 0) {
        printf("%s", help);
    }
    else {
        printf("Unknown command.\n");
    }
}
void sendHTTP404(socklen_t sockfd)
{
	write(sockfd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
}

void sendHTTP403(socklen_t sockfd)
{
	write(sockfd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this CandC webserver.\n</body></html>\n",271);
}

void sendHTTP200(socklen_t sockfd)
{
	char content[MAXLEN];
	sprintf(content,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %d\nConnection: close\nContent-Type: %s\n\n", 1, 0, "text/html");
	write(sockfd, content, strlen(content));

}

void serve_file(char *path, socklen_t sockfd)
{
	int pathlen;
	char * fstr;
	long i, len, ret;
	FILE *f;
	char buffer[MAXLEN];

	pathlen=strlen(path);

	for(int j=0;j<pathlen-1;j++)
		if(path[j] == '.' && path[j+1] == '.') {
			sendHTTP403(sockfd);
			return;
		}
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&path[pathlen-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
		}
	}
	if(fstr == 0){
		sendHTTP403(sockfd);
		return;
	}

	path = path + 1;
 	if(access(path, F_OK) < 0 || strchr(path, '\\')) {
		sendHTTP404(sockfd);
		return;
    }
    else if(access(path, R_OK) < 0) {
		sendHTTP404(sockfd);
		return;
    }
    else {
        f = fopen(path, "rb");

        if (f == NULL) {
			sendHTTP404(sockfd);
			return;
        }
	}

	fseek(f, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
	len = ftell(f);
	fseek(f, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */

	sprintf(buffer,"HTTP/1.1 200 OK\nServer: CandC/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", 1, len, fstr);
	writew(sockfd,buffer,strlen(buffer));

	/* send file in 8KB block - last block may be smaller */
	while (	(ret = fread(buffer, 1, MAXLEN, f)) > 0 ) {
		writew(sockfd,buffer,ret);
	}
}

void serve_botlist(socklen_t sockfd)
{
	char header[MAXLEN];
	char content[MAXLEN + MAXLEN/10];
	char botlist[MAXLEN];
	char addr[INET_ADDRSTRLEN];
	char port[22];

	long content_len = 0;
	struct sockaddr_in bot;
	for (int i = 0; i < nbots; i++)
	{
		bot = bots[i];
		inet_ntopw(AF_INET, &(bot).sin_addr, addr, INET_ADDRSTRLEN);
		sprintf(port, "%u", bot.sin_port);
		sprintf(botlist + content_len, "%s:%s\n", addr, port);
		content_len += strlen(addr) + strlen(port) + 2;
	}
	
	
	sprintf(content, "<html><head>\n<title>Botlist</title>\n</head><body>\n<h1>Botlist</h1>%s</body></html>\n", botlist);
	sprintf(header, "HTTP/1.1 200 OK\nServer: CandC/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: text/html\n\n", 1, strlen(content));
	writew(sockfd, header, strlen(header));
	writew(sockfd, content, strlen(content));

}

void handle_http_req(char *req, socklen_t sockfd)
{
	char *method, *path;
	char *delimiter = " ";

	method = strtok(req, delimiter);
	if (strncmp(method, "GET", 3) != 0) {
		return;
	}
	
	path = strtok(NULL, delimiter);
	printf("%s", path);
	if (!strncmp(path, "/bot/prog_tcp_localhost", 23)) {
		handle_cmd("ptl");
	}
	else if (!strncmp(path, "/bot/prog_tcp", 13)) {
		handle_cmd("pt");
	}
	else if (!strncmp(path, "/bot/prog_udp_localhost", 23)) {
		handle_cmd("pul");
	}
	else if (!strncmp(path, "/bot/prog_udp", 13)) {
		handle_cmd("pu");
	}
	else if (!strncmp(path, "/bot/run2", 9)) {
		handle_cmd("r2");
	}
	else if (!strncmp(path, "/bot/run", 8)) {
		handle_cmd("r");
	}
	else if (!strncmp(path, "/bot/stop", 9)) {
		handle_cmd("s");
	}
	else if (!strncmp(path, "/bot/list", 9)) {
		serve_botlist(sockfd);
	}
	else if (!strncmp(path, "/bot/quit", 9)) {
		handle_cmd("q");
	}
	else{
		serve_file(path, sockfd);
		return;
	}
	sendHTTP200(sockfd);
}
void serve(char * tcp_port)
{
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct addrinfo hints, * res;
	int tsockfd, fdmax, clifd;
	char buf[MAXLEN];
	int nbytes;
	fd_set master, readset;
    char udp_port[] = "5555";

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
    nbots = 0;

	while (1) {
		readset = master;
		printf("> ");
		fflush(stdout);
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
					printf("NEW TCP CONNECTION\n");
					
				}
				else if(i == usockfd) {
					//handle new bot registration
					memset(buf, 0, MAXLEN);
					clilen = sizeof(cli_saddr);
					recvfromw(usockfd, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
					printf("Primio %s\n", buf);
					if (strncmp(buf, "REG", 3) == 0)
					{
                        if (nbots < MAX_NBOTS)
                        {
                            bots[nbots] = cli_saddr;
                            botlens[nbots] = clilen;
                            nbots++;
                        }
                        
					}
				}
				else if(i == STDIN) {
					//handle input from stdin
					memset(buf, 0, MAXLEN);
					read(i, buf, sizeof buf);
                    handle_cmd(buf);
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
						handle_http_req(buf, i);
						
					};
				};
			};
		};
	};

};

int main(int argc, char * argv[])
{
    char tcp_port[22] = "1220";
	if (argc > 2)
    {
        errx(1, "usage: ./CandC [tcp_port]");
    }
    if (argc == 2)
    {
        strcpy(tcp_port, argv[1]);
    }
	serve(tcp_port);
};

