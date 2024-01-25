CC = gcc
CFLAGS = -Wall -pedantic

SERVER = server
CLIENT = client

all: ${SERVER} ${CLIENT}

${SERVER}: ${SERVER}.c copydata.c handleCommand.c shortestQueue.c logger.c


.PHONY: clean
clean:
		rm -f $(SERVER) $(CLIENT) logfile.log