#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "netw.h"

/**
 * @brief Wrapper function for getaddrinfo with error handling.
 * 
 * @param hostname hostname
 * @param servname service or port
 * @param hints hints struct
 * @param res result
 */
void getaddrinfow(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res) {
    int error = getaddrinfo(hostname, servname, hints, res); 
    if (error) errx(1, "%s", gai_strerror(error));
}

void getaddrinfoww(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res) {
    int error = getaddrinfo(hostname, servname, hints, res); 
    if (error) warnx("%s", gai_strerror(error));
}

void getnameinfow(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags) {
    int error = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
    if (error) errx(1, "getnameinfo: %s", gai_strerror(error));
}



void inet_ntopw(int af, const void * src, char * dst, socklen_t size) {
    if(!inet_ntop(af, src, dst, size)) {
        err(1, NULL);
    }
}

void inet_ptonw(int af, const char * src, void * dst) {
    if (inet_pton(af, src, dst) != 1) { 
        err(1, NULL);
    }
}

int socketw(int family, int type, int proto) {
    int sock = socket(family, type, proto);
    if(sock < 0) {
        errx(1, "Unable to create client socket.");
    }
    return sock;
}

ssize_t sendtow(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen){
    ssize_t n = sendto(sockfd, buff, nbytes, flags, to, addrlen);
    if(n < 0)
        err(1, NULL);
    return n;
}

ssize_t recvfromw(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen) {
    ssize_t n = recvfrom(sockfd, buff, nbytes, flags, from, fromaddrlen);
    if(n < 0)
        err(1, NULL);
    return n;
}

void bindw(int sockfd, const struct sockaddr *myaddr, int addrlen) {
    int error = bind(sockfd, myaddr, addrlen);
    if (error < 0) {
        errx(1, NULL);
    }
}

struct servent* getservbynamew(const char * name, const char * proto) {
    struct servent* srv= getservbyname(name,"udp");
    if(srv == NULL){
        errx(1, "Unknown service: %s", name);
    }
    return srv;
}

struct addrinfo* resolveipandnport(char *address, char *s_port, int ai_socktype) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = ai_socktype;
	getaddrinfoww(address, s_port, &hints, &res);
    return res;
}
