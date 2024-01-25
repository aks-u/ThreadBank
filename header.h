#ifndef HEADER_H
#define HEADER_H

#define SOCKET_PATH "./sockets/main-socket"
#define NUM_DESKS 10

typedef struct {
  int id;
  int queueSize;
  char* path;
} Desk;

int handleCommand(char* buf, int clientFD);

void copydata(int from,int to);

Desk* shortestQueue();

#endif