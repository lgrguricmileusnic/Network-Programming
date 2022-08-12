#CC = cc
CFLAGS = -Wall -g
LDFLAGS =

POMOCNI_C = netw.c
POMOCNI_O = ${POMOCNI_C:.c=.o} 
HEADERS = netw.h

PRVI_C = bot.c
PRVI_O = ${PRVI_C:.c=.o}

DRUGI_C = UDP_server.c
DRUGI_O = ${DRUGI_C:.c=.o}

all: prvi drugi

prvi: $(PRVI_O) $(POMOCNI_O)
	$(CC) $(PRVI_O) $(POMOCNI_O) -o ${PRVI_C:.c=}
drugi: $(DRUGI_O) $(POMOCNI_O)
	$(CC) $(DRUGI_O) $(POMOCNI_O) -o ${DRUGI_C:.c=}
prvi.o: $(HEADERS) 

drugi.o: $(HEADERS) 

$(POMOCNI_O): $(HEADERS)

clean:
		-rm -f bot UDP_server *.o *.core