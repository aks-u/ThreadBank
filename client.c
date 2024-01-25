#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define SOCKET_PATH "./sockets/main-socket"

// Function to send commands to the desk
void copydata(int from,int to) {
   char buf[1024];
   int amount;
   char response[50];
   
   while ((amount = read(from, buf, sizeof(buf))) > 0) {
      assert((write(to, buf, amount) == amount));

      // Receive a response message from the desk (ok/fail)
      ssize_t bytes_received = recv(to, response, sizeof(response), 0);
      response[bytes_received] = '\0';

      // Print the response to the standard output
      printf("%s\n", response);
      fflush(stdout);
      
      // Break the loop after the desk responds with ok to command 'q'
      if (strcmp(response, "ok: Quit") == 0) {
         break;
      } 
   }
   assert(amount >= 0);
}


int main(void) {
   struct sockaddr_un mainAddress, deskAddress;
   int sock, deskSock;
   size_t addrLength;

   // Main socket
   assert((sock = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0);

   mainAddress.sun_family = AF_UNIX;
   strcpy(mainAddress.sun_path, SOCKET_PATH);
   addrLength = sizeof(mainAddress.sun_family) + strlen(mainAddress.sun_path);

   // Connect to the main server
   assert((connect(sock, (struct sockaddr *)&mainAddress, addrLength)) == 0);

   // Receive the path to the desk socket
   char path[30];
   ssize_t bytes_received = recv(sock, path, sizeof(path), 0);
   path[bytes_received] = '\0';

   // Close the main socket
   close(sock);



   // Desk socket
   assert((deskSock = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0);

   deskAddress.sun_family = AF_UNIX;
   strcpy(deskAddress.sun_path, path);
   addrLength = sizeof(deskAddress.sun_family) + strlen(deskAddress.sun_path);

   // Connect to the desk server
   assert((connect(deskSock, (struct sockaddr *)&deskAddress, addrLength)) == 0);   

   // Send a message indicating that this is a client
   int client = 0;
   send(deskSock, &client, sizeof(int), 0);

   // Receive "ready" flag to know that the client has reached the desk and is being serviced
   int ready;
   recv(deskSock, &ready, sizeof(int), 0);

   // If the server is ready, start sending commands with copydata
   if (ready == 1) {
      printf("ready\n");
      fflush(stdout);
      copydata(STDIN_FILENO, deskSock);
      close(deskSock);
   }
   
   return 0;
}
