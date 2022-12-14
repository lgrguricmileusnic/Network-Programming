
void getaddrinfow(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
void getaddrinfoww(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
void getnameinfow(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
void inet_ntopw(int af, const void * src, char * dst, socklen_t size);
void inet_ptonw(int af, const char * src, void * dst);
int socketw(int family, int type, int proto);
ssize_t sendtow(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen);
ssize_t recvfromw(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen);
void bindw(int sockfd, const struct sockaddr *myaddr, int addrlen);
int listenw(int sockfd, int backlog);
int acceptw(int sockfd, struct sockaddr* cliaddr, socklen_t *addrlen);
struct servent* getservbynamew(const char * name, const char * proto);
struct addrinfo* resolveipandnport(char *address, char *s_port, int ai_socktype);
int connectw(int sockfd, const struct sockaddr *server, socklen_t addrlen);
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t writenw(int fd, const void *vptr, size_t n);
ssize_t readnw(int fd, void *vptr, size_t n);
int readw(int fd, char *buf, int max);
int writew(int fd, char *buf, int num);
ssize_t sendw(int sockfd, const void *msg, size_t len, int flags);
ssize_t recvw(int sockfd, void *msg, size_t len, int flags);
void setsockoptw(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
