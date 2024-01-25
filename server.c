#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#include "header.h"
#include "logger.h"


pthread_t deskThreads[NUM_DESKS];
Desk deskArray[NUM_DESKS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

char logMessage[100];

int bankOpen = 1;

// Signal handler for SIGINT
void signalHandler(int sig) {
  if (sig == SIGINT) {
    bankOpen = 0;
  }
}


// Function executed by each desk thread
void* deskFunction(void* arg) {
  Desk* desk = (Desk*)arg;

  // Set up Unix domain socket for the desk
  struct sockaddr_un deskAddress;
  int deskSock, deskConn;
  socklen_t deskAddrLength;

  assert((deskSock = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0);
  unlink(desk->path);

  deskAddress.sun_family = AF_UNIX;
  strcpy(deskAddress.sun_path, desk->path);

  deskAddrLength = sizeof(deskAddress.sun_family) + strlen(deskAddress.sun_path);
  assert(bind(deskSock, (struct sockaddr *)&deskAddress, deskAddrLength) == 0);
  assert(listen(deskSock, 5) == 0);

  pthread_mutex_lock(&logMutex);
  memset(logMessage, 0, sizeof(logMessage));
  sprintf(logMessage, "Desk %d waiting for clients.", desk->id);
  logger(logMessage);
  pthread_mutex_unlock(&logMutex);

  int isBank;
  int ready = 1;
  while (1) {
    // Accept connection from clients
    deskConn = accept(deskSock, (struct sockaddr *) &deskAddress, &deskAddrLength);
    recv(deskConn, &isBank, sizeof(int), 0);
    
    // If the connection is from the bank, break from loop and finish thread execution
    if (isBank) {
      break;
    }

    pthread_mutex_lock(&logMutex);
    memset(logMessage, 0, sizeof(logMessage));
    sprintf(logMessage, "Client received at desk %d.", desk->id);
    logger(logMessage);
    pthread_mutex_unlock(&logMutex);
    
    // Send "ready" flag to signal the client that they have reached the desk and are being serviced
    send(deskConn, &ready, sizeof(int), 0);
    copydata(deskConn, STDOUT_FILENO);        // Function for reading commands from the client
    close(deskConn);      // After client has finished, close the connection

    pthread_mutex_lock(&logMutex);
    memset(logMessage, 0, sizeof(logMessage));
    sprintf(logMessage, "Client finished at desk %d.", desk->id);
    logger(logMessage);
    pthread_mutex_unlock(&logMutex);


    // Update the desk's queue size
    pthread_mutex_lock(&mutex);
    desk->queueSize--;
    pthread_mutex_unlock(&mutex);
  }

  printf("\nClosing desk %s", desk->path);
  return NULL;
}



// Main function
int main(void) {
  logger("-------------------------------------------");
  logger("Bank opened.");
  
  // Set up signal handler for SIGINT
  struct sigaction sig;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags = 0;
	sig.sa_handler = signalHandler;
  sigaction(SIGINT, &sig, NULL);

  // Initialize desks and create desk threads
  for (int i = 0; i < NUM_DESKS; i++) {
    deskArray[i].id = i;
    deskArray[i].queueSize = 0;

    char addr[30];
    sprintf(addr, "./sockets/desk%d", i);
    deskArray[i].path = malloc(strlen(addr) + 1);
    strcpy(deskArray[i].path, addr);

    pthread_create(&deskThreads[i], NULL, deskFunction, (void*)&deskArray[i]);

    pthread_mutex_lock(&logMutex);
    memset(logMessage, 0, sizeof(logMessage));
    sprintf(logMessage, "Desk %d opened", i);
    logger(logMessage);
    pthread_mutex_unlock(&logMutex);
  }

  // Set up the main socket
  struct sockaddr_un address;
  int sock, conn;
  socklen_t addrLength;

  assert((sock = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0);

  unlink(SOCKET_PATH);

  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, SOCKET_PATH);

  addrLength = sizeof(address.sun_family) + strlen(address.sun_path);

  assert(bind(sock, (struct sockaddr *) &address, addrLength) == 0);
  assert(listen(sock, 5) == 0);

  pthread_mutex_lock(&logMutex);
  logger("Server waiting for clients");
  pthread_mutex_unlock(&logMutex);

  // Accept client connections and assign them to the desk with the shortest queue
  while (bankOpen) {
    conn = accept(sock, (struct sockaddr *)&address, &addrLength);

    pthread_mutex_lock(&mutex);

    Desk* shortestQueueDesk = shortestQueue();
    char* shortestQueuePath = shortestQueueDesk->path;
    send(conn, shortestQueuePath, strlen(shortestQueuePath), 0);
    shortestQueueDesk->queueSize++;

    pthread_mutex_unlock(&mutex);
    
    close(conn);
  }

  pthread_mutex_lock(&logMutex);
  logger("SIGINT caught. Bank is closing.");
  pthread_mutex_unlock(&logMutex);

  // Close desk threads
  for (int i = 0; i < NUM_DESKS; i++) {
    struct sockaddr_un deskAddress;
    int deskSock;
    assert((deskSock = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0);
    deskAddress.sun_family = AF_UNIX;
    strcpy(deskAddress.sun_path, deskArray[i].path);
    addrLength = sizeof(deskAddress.sun_family) + strlen(deskAddress.sun_path);
    assert((connect(deskSock, (struct sockaddr *)&deskAddress, addrLength)) == 0);

    int bank = 1;
    send(deskSock, &bank, sizeof(int), 0);    // Signal the desks that the connection is from the bank

    pthread_join(deskThreads[i], NULL);
    close(deskSock);

    pthread_mutex_lock(&logMutex);
    memset(logMessage, 0, sizeof(logMessage));
    sprintf(logMessage, "Desk %d closed", i);
    logger(logMessage);
    pthread_mutex_unlock(&logMutex);
  }

  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&logMutex);

  printf("\nBank closed\n");
  logger("Bank closed.");
  return 0;
}
