void getaddrinfoww(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
void getaddrinfow(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
void getnameinfow(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
void inet_ntopw(int af, const void * src, char * dst, socklen_t size);
void inet_ptonw(int af, const char * src, void * dst);
int socketw(int family, int type, int proto);
ssize_t sendtow(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen);
ssize_t recvfromw(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen);
void bindw(int sockfd, const struct sockaddr *myaddr, int addrlen);
struct servent* getservbynamew(const char * name, const char * proto);
struct addrinfo* resolveipandnport(char *address, char *s_port, int ai_socktype);