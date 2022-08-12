#CC = cc
CFLAGS = -Wall -g
LDFLAGS = -pthread

POMOCNI_C = netw.c
POMOCNI_O = ${POMOCNI_C:.c=.o} 
HEADERS = netw.h

PRVI_C = bot.c
PRVI_O = ${PRVI_C:.c=.o}

DRUGI_C = server.c
DRUGI_O = ${DRUGI_C:.c=.o}

TRECI_C = CandC.c
TRECI_O = ${TRECI_C:.c=.o}

all: prvi drugi treci

prvi: $(PRVI_O) $(POMOCNI_O)
	$(CC) $(LDFLAGS) $(PRVI_O) $(POMOCNI_O) -o ${PRVI_C:.c=}
drugi: $(DRUGI_O) $(POMOCNI_O)
	$(CC) $(LDFLAGS) $(DRUGI_O) $(POMOCNI_O) -o ${DRUGI_C:.c=}

treci: $(TRECI_O) $(POMOCNI_O)
	$(CC) $(LDFLAGS) $(TRECI_O) $(POMOCNI_O) -o ${TRECI_C:.c=}

prvi.o: $(HEADERS) 

drugi.o: $(HEADERS) 

treci.o: $(HEADERS) 

$(POMOCNI_O): $(HEADERS)

clean:
		-rm -f bot server CandC *.o *.core