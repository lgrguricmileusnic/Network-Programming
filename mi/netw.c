#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "netw.h"

#define readn(fd, ptr, n) recv(fd, ptr, n, MSG_WAITALL)

/**
 * @brief Wrapper function for getaddrinfo with error handling.
 * 
 * @param hostname hostname
 * @param servname service or port
 * @param hints hints struct
 * @param res result
 */
void getaddrinfow(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    int error = getaddrinfo(hostname, servname, hints, res); 
    if (error) errx(1, "%s", gai_strerror(error));
}

void getaddrinfoww(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    int error = getaddrinfo(hostname, servname, hints, res); 
    if (error) warnx("%s", gai_strerror(error));
}

void getnameinfow(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
    int error = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
    if (error) errx(1, "getnameinfo: %s", gai_strerror(error));
}



void inet_ntopw(int af, const void * src, char * dst, socklen_t size)
{
    if(!inet_ntop(af, src, dst, size)) {
        err(1, NULL);
    }
}

void inet_ptonw(int af, const char * src, void * dst)
{
    if (inet_pton(af, src, dst) != 1) { 
        err(1, NULL);
    }
}

int socketw(int family, int type, int proto)
{
    int sock = socket(family, type, proto);
    if(sock < 0) {
        errx(1, "Unable to create client socket.");
    }
    return sock;
}

ssize_t sendtow(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen)
{
    ssize_t n = sendto(sockfd, buff, nbytes, flags, to, addrlen);
    if(n < 0)
        err(1, NULL);
    return n;
}

ssize_t recvfromw(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen)
{
    ssize_t n = recvfrom(sockfd, buff, nbytes, flags, from, fromaddrlen);
    if(n < 0)
        err(1, NULL);
    return n;
}

void bindw(int sockfd, const struct sockaddr *myaddr, int addrlen)
{
    int error = bind(sockfd, myaddr, addrlen);
    if (error < 0) {
        errx(1, NULL);
    }
}

int listenw(int sockfd, int backlog)
{
    int error = listen(sockfd, backlog);
    if (error < 0) {
        errx(1, NULL);
    }
    return error;
}

int acceptw(int sockfd, struct sockaddr* cliaddr, socklen_t *addrlen)
{
    int error = accept(sockfd, cliaddr, addrlen);
    if(error < 0) {
        errx(1, NULL);
    }
    return error;
}

struct servent* getservbynamew(const char * name, const char * proto)
{
    struct servent* srv= getservbyname(name,"udp");
    if(srv == NULL){
        errx(1, "Unknown service: %s", name);
    }
    return srv;
}

struct addrinfo* resolveipandnport(char *address, char *s_port, int ai_socktype)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = ai_socktype;
	getaddrinfoww(address, s_port, &hints, &res);
    return res;
}

int connectw(int sockfd, const struct sockaddr *server, socklen_t addrlen)
{
    int error = connect(sockfd, server, addrlen);
    if (error < 0)
        errx(1, "Unable to connect to server: ");
    return error;
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft; 
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return (-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

ssize_t writenw(int fd, const void *vptr, size_t n)
{
    int error = writen(fd, vptr, n);
    if (error < 0)
        errx(1, "Encountered problem during write: ");
    return error;
}

ssize_t readnw(int fd, void *vptr, size_t n)
{
    int error = readn(fd, vptr, n);
    if (error < 0)
        errx(1, "Encountered problem during read: ");
    return error;
}

int readw(int fd, char *buf, int max)
{
    int error = read(fd, buf, max);
    if (error < 0)
        err(1, NULL);
    return error;
}

int writew(int fd, char *buf, int num)
{
    int error = write(fd, buf, num);
    if (error < 0)
        errx(1, "Encountered problem during write: ");
    return error;
}

ssize_t sendw(int sockfd, const void *msg, size_t len, int flags)
{
    ssize_t error = send(sockfd, msg, len, flags);
        if (error < 0)
        errx(1, "Encountered problem during send: ");
    return error;
}

ssize_t recvw(int sockfd, void *msg, size_t len, int flags)
{
    ssize_t error = recv(sockfd, msg, len, flags);
    if (error < 0)
        errx(1, "Encountered problem during recv: ");
    return error;
}

void setsockoptw(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    int error = setsockopt(sockfd, IPPROTO_IP, IP_TTL, optval, optlen);
    if (error < 0)
        errx(1, "setsockopt fail: ");
}
