#CC = cc
CFLAGS = -Wall -g
LDFLAGS = -pthread

POMOCNI_C = netw.c
POMOCNI_O = ${POMOCNI_C:.c=.o} 
HEADERS = netw.h

PRVI_C = knock_server.c
PRVI_O = ${PRVI_C:.c=.o}

all: prvi

prvi: $(PRVI_O) $(POMOCNI_O)
	$(CC) $(LDFLAGS) $(PRVI_O) $(POMOCNI_O) -o ${PRVI_C:.c=}


prvi.o: $(HEADERS) 

$(POMOCNI_O): $(HEADERS)

clean:
		-rm -f knock_server *.o *.core