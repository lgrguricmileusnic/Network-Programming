#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <err.h>
#include "netw.h"

#define MAXLEN 256
#define NPORTS 5
#define U1 0
#define U2 1
#define BACKLOG 10
#define MSG "Hello world!"

int timeout = 10;
char lozinka[MAXLEN];
char tajni_kljuc[3];
char challenge[3];
char check[3];
char *uports[NPORTS];

void generate_new_challenge()
{
	challenge[0] = (int)(((float)rand() / (float)RAND_MAX) * 25);
	challenge[1] = (int)(((float)rand() / (float)RAND_MAX) * 25);
	for (int i = 0; i < 2; i++)
	{
		if ((((float)rand() / (float)RAND_MAX)) > 0.5)
		{
			challenge[i] += 0x41;
		}
		else
		{
			challenge[i] += 0x61;
		}
		check[i] = challenge[i] ^ tajni_kljuc[i];
	}
}

void tcp_app()
{
	struct addrinfo hints, *res;
	int tsockfd, clifd;
	fd_set readset;
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct timeval tv = {timeout, 0};
	
	memset( & hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	printf("Pokrecem tcp aplikaciju.\n");
	getaddrinfow(NULL, "1234", &hints, &res);
	tsockfd = socketw(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
	bindw(tsockfd, res -> ai_addr, res -> ai_addrlen);
	listenw(tsockfd, BACKLOG);

	FD_ZERO( &readset);
	FD_SET(tsockfd, &readset);


	selectw(tsockfd + 1, &readset, NULL, NULL, &tv);
	if(FD_ISSET(tsockfd, &readset))
	{
		clilen = sizeof cli_saddr;
		clifd = acceptw(tsockfd, (struct sockaddr * ) &cli_saddr, &clilen);
		sendw(clifd, MSG, strlen(MSG), 0);
		close(clifd);
	}
	else {
		printf("Timeout istekao!\n");
	}
	close(tsockfd);

}

void serve()
{
	struct sockaddr_in cli_saddr;
	socklen_t clilen;
	struct addrinfo hints, *res;
	int fdmax;
	int usocks[NPORTS];
	char buf[MAXLEN];
	fd_set umaster, ureadset1, ureadset2;
	struct timeval tv = {timeout, 0};
	FD_ZERO( & umaster);
	FD_ZERO( & ureadset1);

	srand(time(NULL));
	memset( & hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;

	for (int i = 0; i < NPORTS; i++)
	{
		getaddrinfow(NULL, uports[i], &hints, &res);
		usocks[i] = socketw(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
		bindw(usocks[i], res -> ai_addr, res -> ai_addrlen);
		FD_SET(usocks[i], & umaster);
	}

	fdmax = usocks[NPORTS - 1];
	while (1)
	{
		ureadset1 = umaster;
		selectw(fdmax + 1, &ureadset1, NULL, NULL, NULL);
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &ureadset1)) {
				if (i == usocks[U1])
				{
					memset(buf, 0, MAXLEN);
					clilen = sizeof(cli_saddr);
					recvfromw(usocks[U1], buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
					printf("Primio %s na u1.\n", buf);
					if (strncmp(buf, lozinka, strlen(lozinka)) == 0)
					{
						generate_new_challenge();
						printf("check: %s\n", check);
						printf("challenge: %s\n", challenge);
						sendtow(usocks[U1], challenge, strlen(challenge), 0, (struct sockaddr *) &cli_saddr, sizeof(cli_saddr));
						ureadset2 = umaster;
						printf("Timer pokrenut.\n");
						tv.tv_sec = timeout; // dodan timer reset
						selectw(fdmax + 1, &ureadset2, NULL, NULL, &tv);
						if(tv.tv_sec == 0 && tv.tv_usec == 0)
						{
							printf("Timeout istekao!\n");
						}
						else
						{
							for (int j = 0; j <= fdmax; j++)
							{
								if (FD_ISSET(j, &ureadset2)){
									if (j != usocks[U2]) {
										printf("Primio poruku na krivi port. (Nije u2).\n");
									}
									else {
										memset(buf, 0, MAXLEN);
										recvfromw(usocks[U2], buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
										printf("Primio %s na u2\n", buf);
										if (strncmp(buf, check, 2) == 0)
										{
											tcp_app();
										}
										else
										{
											printf("Pogresna poruka na u2.\n");
										}
									}
								}
							}
						}
					};
				}
				else {
					memset(buf, 0, MAXLEN);
					clilen = sizeof(cli_saddr);
					recvfromw(i, buf, MAXLEN, 0, (struct sockaddr *) &cli_saddr, &clilen);
					printf("Primio %s na port koji nije u1\n", buf);
				}
			}
		}
	}
	
}

int main(int argc, char * argv[])
{
	int opt;
	
	const char optstr[] = "t:";
	char *endptr = NULL;

	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 't':
			timeout = strtol(optarg, &endptr, 10);
			if (timeout <= 0 || errno == EINVAL || errno == ERANGE)
			{
				err(1, "Invalid timeout value.\n");
			}
			break;
		}
	}
	if (argc - optind != 7)
	{
		printf("usage: knock_server [-t timeout] lozinka tajni_kljuc u1 u2 u3 u4 u5\n");
		exit(1);
	}
	strcpy(lozinka, argv[optind++]);
	strcpy(tajni_kljuc, argv[optind++]);
	int i = 0;
	while(optind < argc)
	{
		uports[i] = calloc(22, 1);
		strcpy(uports[i], argv[optind]);
		i++; optind++;
	}

	serve();
}