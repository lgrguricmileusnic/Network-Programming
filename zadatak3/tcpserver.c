#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <err.h>
#include "netw.h"

#define BACKLOG 10
#define MAXREAD 100
#define MAXFILENAME 4096
#define CHUNKSIZE 32

#define SRVERROR "SERVER_ERROR"
#define NOTFOUND "NOT_FOUND"
#define BADPERMISSIONS "BAD_PERMISSIONS"


void parse_opt(char* sport, int argc, char *argv[])
{
    int opt;
    const char optstr[] = "p:";
    while ((opt = getopt(argc, argv, optstr)) != -1) {
        if (opt == 'p') {
            strcpy(sport, optarg);
            break;
        } else {
            errx(1, "Usage:  ./tcpserver [-p port]");
        }
	}
}
void senderr(uint8_t type, int sockfd) {
    void *msg;
    size_t msglen = sizeof(uint8_t);

    switch(type) {
        case 1:
            msglen += strlen(NOTFOUND);
            msg = malloc(msglen);
            memcpy(msg, &type, 1);
            memcpy(msg + 1, NOTFOUND, strlen(NOTFOUND));
            sendw(sockfd, msg, msglen, 0);
            break;
        case 2:
            msglen += strlen(BADPERMISSIONS);
            msg = malloc(msglen);
            memcpy(msg, &type, 1);
            memcpy(msg + 1, BADPERMISSIONS, strlen(BADPERMISSIONS));
            sendw(sockfd, msg, msglen, 0);
            break;
        case 3:
            msglen += strlen(SRVERROR);
            msg = malloc(msglen);
            memcpy(msg, &type, 1);
            memcpy(msg + 1, SRVERROR, strlen(SRVERROR));
            sendw(sockfd, msg, msglen, 0);
            break;
    }
}
void check_and_respond(int sockfd, char *filename, int offset)
{
    FILE *f;

    if(access(filename, F_OK) < 0 || strchr(filename, '\\')) {
        senderr(0x01, sockfd);
        return;
    }
    else if(access(filename, R_OK) < 0) {
        senderr(0x02, sockfd);
        return;
    }
    else {
        f = fopen(filename, "rb");
        int nbytes;
        char buf[CHUNKSIZE];
        uint8_t type = 0x00;

        if (f == NULL) {
            senderr(0x03, sockfd);
            return;
        }
        sendw(sockfd, &type, 1, 0);
        fseek(f, offset, SEEK_SET);
        while(1) {
            nbytes = fread(buf, 1, CHUNKSIZE, f);
            sendw(sockfd, buf, nbytes, 0);
            if(nbytes < CHUNKSIZE)
                break;
        }
        fclose(f);
    }
    return;
}

int main (int argc, char *argv[]) 
{
	char sport[] = "1234";
    char buf[MAXREAD];
    int sockfd, clifd;
    struct addrinfo hints, *res;
    struct sockaddr cli_addr;
    socklen_t cli_size;

    parse_opt(sport, argc, argv);
    if(argc - optind != 0)
        errx(1, "Usage:  ./tcpserver [-p port]");

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags |= AI_PASSIVE;

    getaddrinfow(NULL, sport, &hints, &res);
    sockfd = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);

    bindw(sockfd, res->ai_addr, res->ai_addrlen);
    listenw(sockfd, BACKLOG);
    memset(&buf, 'x', MAXREAD);
    while(1) {
        char filename[MAXFILENAME];
        int nbytes, fnsize;
        uint32_t noffset, offset;
        clifd = acceptw(sockfd, &cli_addr, &cli_size);

        if (readnw(clifd, buf, 4) != 4)
            errx(1, "Error reading offset from client.");

        memcpy(&noffset, buf, 4);
        offset = ntohl(noffset);

        nbytes = recvw(clifd, buf, MAXREAD - 1, 0);
        memcpy(filename, buf, nbytes);
        fnsize = nbytes;
        while(buf[nbytes - 1] != 0 || (strlen(filename) == 0)) {
            memset(&buf, 'x', MAXREAD);
            nbytes = recvw(clifd, buf, MAXREAD - 1, 0);
            memcpy(filename + fnsize, &buf, nbytes);
            fnsize += nbytes;
        }
        check_and_respond(clifd, filename, offset);
        close(clifd);
    }
    close(sockfd);
}