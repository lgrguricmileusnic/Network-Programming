#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<err.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<errno.h>

/**
 * @brief Fills passed flags array according to selected options and checks for option incompatibility.
 * flag == 0 - option default, 
 *      >  0 - option set, 
 *      <  0 - alternative option set 
 * Options: [t|u,x,h|n,4|6,r]
 * 
 * @param flags integer array of length 5
 * @param argc argument count
 * @param argv argument vector
 */
void parse_options(int flags[], int argc, char *argv[])
{
    int opt;
    const char optstr[] = "tuxhn46r";
    if(flags == NULL) {
        flags = (int *) calloc(sizeof(int), 5);
    }
    
    while((opt = getopt(argc, argv, optstr)) != -1) {
        switch(opt) {
            case 't':
                if(flags[0] < 0) {
                    err(1, "Flags -t and -u are incompatible");
                }
                flags[0]++;
                break;
            case 'u':
                if(flags[0] > 0) {
                    err(1, "Flags -t and -u are incompatible");
                }
                flags[0]--;
                break;
            case 'x':
                flags[1]++;
                break;
            case 'h':
                if(flags[2] < 0) {
                    err(1, "Flags -h and -n are incompatible");
                }
                flags[2]++;
                break;
            case 'n':
                if(flags[2] > 0) {
                    err(1, "Flags -h and -n are incompatible");
                }
                flags[2]--;
                break;
            case '4':
                if(flags[3] < 0) {
                    err(1, "Flags -4 and -6 are incompatible");
                }
                flags[3]++;
                break;
            case '6':
                if(flags[3] > 0) {
                    err(1, "Flags -4 and -6 are incompatible");
                }
                flags[3]--;
                break;
            case 'r':
                flags[4]++;
                break;
        }
    }
}

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

void getnameinfow(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags){
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

/**
 * @brief Reverses passed string.
 * 
 * @param str string to be reversed
 */
void strrev(char str[])
{
  int len = strlen(str);

  for (int i = 0; i < len / 2; i++)
  {
    char ch = str[i];
    str[i] = str[len - i - 1];
    str[len - i - 1] = ch;
  }
}

/**
 * @brief Converts short integer to 2 byte hex string representation.
 * 
 * @param dec integer to be converted
 * @return char* 2 byte hex string representation
 */
char* dtohs(int16_t dec)
{
    int rm, j = 0;
    char hex[5];
    char *res = (char *) malloc(4);
    while (dec != 0)
    {
        rm = dec % 16;
        if (rm < 10)
            hex[j++] = 48 + rm;
        else
            hex[j++] = 55 + rm;
        dec = dec / 16;
    }
    strrev(hex);
    int len = strlen(hex);
    if(len < 4){
        sprintf(res, "%0*d%s", (int) (4 - len), 0, hex);
    }
    return res;
}

/**
 * @brief Formats passed addrinfo, address and port as
 *        address (cannonical name) port
 * 
 * If hex_pres is set, port will be represented as hex.
 * If host_order_f is set host order will be used for port, network order otherwise.
 * 
 * @param res addrinfo
 * @param addr address
 * @param port port
 * @param host_order_f host order flag
 * @param hex_f hex presentation flag
 */
void formatted_print(struct addrinfo *res, char * addr, in_port_t port, int host_order_f, int hex_f) {
    char addrstr[100];
    char *fport = (char *) malloc(4);
    if(host_order_f >= 0) {  // if host order
        port = ntohs(port);
    }
    if(hex_f) {
        fport = dtohs(port);
    } else {
        sprintf(fport, "%d", port);
    }
    inet_ntopw(res->ai_family, addr, addrstr, 100);
    printf("%s (%s) %s\n", addrstr, res->ai_canonname, fport);
}

/**
 * @brief Main function
 * 
 * @param argc argument count
 * @param argv argument vector
 * @return int status code
 */
int main (int argc, char *argv[])
{   
    int flags[5] = {0}; //flag ==0 - default, >0 - option set, <0 - alternative option set, options: [t|u,x,h|n,4|6,r]
    
    parse_options(flags, argc, argv);

    if((argc - optind) != 2) {
        err(1, "usage: prog [-r] [-t|-u] [-x] [-h|-n] [-46] {hostname|IP_address} {servicename|port}");
    }

    char* host = argv[optind++];
    char* service = argv[optind];

    if(flags[4] == 0) { //normal lookup
        struct addrinfo hints, *res, *temp;

        memset(&hints, 0, sizeof(hints));
    
        hints.ai_protocol = flags[0] >= 0 ? IPPROTO_TCP : IPPROTO_UDP;
        hints.ai_family = flags[3] >= 0 ? AF_INET : AF_INET6;
        hints.ai_flags |= AI_CANONNAME;
        //getaddrinfo(host, service, &hints,&res);
        getaddrinfow(host, service, &hints,&res);

        temp = res;
        if(flags[3] >= 0) { //IPv4
            struct sockaddr_in *saddr;
            while(res) {
            saddr = (struct sockaddr_in *) res->ai_addr;
            formatted_print(res, (char *)&(saddr)->sin_addr, saddr->sin_port, flags[2], flags[1]);
            res = res->ai_next;
            }
        } else { //IPv6
            struct sockaddr_in6 *saddr6;
            while(res) {
            saddr6 = (struct sockaddr_in6 *) res->ai_addr;
            formatted_print(res, (char *)&(saddr6)->sin6_addr, saddr6->sin6_port, flags[2], flags[1]);
            res = res->ai_next;
            }
        }
        freeaddrinfo(temp);
        
    } else { //reverse lookup
        char res_host[NI_MAXHOST];
        char res_serv[NI_MAXSERV];
        struct servent * srv;
        in_port_t nport = htons(strtol(service, NULL, 10));
        srv = getservbyport(nport, flags[0] < 0 ? "udp" : "tcp");
        if(flags[3] >= 0) { //IPv4
            struct sockaddr_in saddr;
            
            saddr.sin_family = AF_INET;
            saddr.sin_port = nport;

            inet_ptonw(AF_INET, host, &(saddr.sin_addr));
            int in_flags = NI_NAMEREQD;
            if(flags[0] < 0) in_flags |= NI_DGRAM;

            getnameinfow((struct sockaddr *)&saddr, sizeof(struct sockaddr_in), res_host, sizeof(res_host), res_serv, NI_MAXSERV, in_flags);
            printf("%s (%s) %s\n", host, res_host, srv->s_name); 
        } else { //IPv6
            struct sockaddr_in6 saddr6;
            saddr6.sin6_family = AF_INET6;
            saddr6.sin6_port = strtol(service, NULL, 10);
            inet_ptonw(AF_INET6, host, &(saddr6.sin6_addr));
            int in_flags = NI_NAMEREQD;
            if(flags[0] < 0) in_flags |= NI_DGRAM;
            
            getnameinfow((struct sockaddr *)&saddr6, sizeof(struct sockaddr_in6), res_host, sizeof(res_host), res_serv, NI_MAXSERV, in_flags);
            printf("%s (%s) %s\n", host, res_host, srv->s_name); 
        }
    }

    return 0;
    
}

