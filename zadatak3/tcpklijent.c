#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "netw.h"

#define CHUNKSIZE 32

void parse_opt(char *sport, char *saddress, int *resume, int argc, char *argv[])
{
    int opt;
    const char optstr[] = "p:s:c";
    while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt)
		{
			case 'p':
				strcpy(sport, optarg);
				break;
			case 's':
                strcpy(saddress, optarg);
				break;
            case 'c':
                *resume = 1;
                break;
            default:
                err(1, "Usage:  ./tcpklijent [-s server] [-p port] [-c] filename");
		}	
	}
}

int file_check(char *filename, int resume)
{
    struct stat st;
    if(resume == 1) {
        if(access(filename, F_OK) < 0) 
            errx(1, "File %s doesn't exist.", filename);
        if(access(filename, W_OK) < 0)
            errx(1, "User doesn't have write permission for file %s.", filename);
        stat(filename, &st);
        return st.st_size;
    }
    if(!access(filename, F_OK)) 
        errx(1, "File %s already exists and option -c not present.", filename);
    return 0;
}

void create_and_send(int sockfd, char *filename, uint32_t offset)
{
    void *msg;
    size_t msglen = sizeof(offset) + strlen(filename) + 1;
    uint32_t noffset = htonl(offset);


    msg = malloc(msglen);

    memcpy(msg, &noffset, 4);
    memcpy(msg + sizeof(noffset), filename, strlen(filename) + 1);
    sendw(sockfd, msg, msglen, 0);
}

int main (int argc, char *argv[])
{
    char filename[4096];
    FILE* fd;
    int resume = 0;
    uint32_t offset;
    char sport[] = "1234";
    char server[] = "127.0.0.1";
    int sockfd;
    struct addrinfo hints, *res;
    struct sockaddr_in serv_addr;

    parse_opt(sport, server, &resume, argc, argv);
    if((argc - optind) != 1) {
        err(1, "Usage:  ./tcpklijent [-s server] [-p port] [-c] filename");
    }
    
    strcpy(filename, argv[optind]);

    offset = file_check(filename, resume);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    getaddrinfow(server, sport, &hints, &res);
    sockfd = socketw(res->ai_family, res->ai_socktype, res->ai_protocol);
    serv_addr.sin_addr = ((struct sockaddr_in*) res->ai_addr)->sin_addr;
    serv_addr.sin_port = ((struct sockaddr_in*) res->ai_addr)->sin_port;
    serv_addr.sin_family = res->ai_family;

    connectw(sockfd, ((struct sockaddr*) &serv_addr), res->ai_addrlen);
    create_and_send(sockfd, filename, offset);

    char buf[CHUNKSIZE];
    uint8_t type = 4;
    readnw(sockfd, &type, 1);
    if (type == 4) {
        errx(1, "Error getting file");
    }
    else if (type == 0x00) {
        fd = fopen(filename, "ab");
        //fseek(fd, 0, SEEK_END);
        while(1) {
            int nbytes = readw(sockfd, buf, CHUNKSIZE);
            if(nbytes == 0) 
                break;
            fwrite(buf, 1, nbytes, fd);
        }
        fclose(fd);
    }
    else if (type == 0x01) {
        errx(1, "File doesn't exist.");
    }
    else if (type == 0x02) {
        errx(2, "Server doesn't have read permissions for specified file.");
    }
    else if (type == 0x03) {
        errx(3, "Internal server error");
    }

    close(sockfd);
    return 0;
}