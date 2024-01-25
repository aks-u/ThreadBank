#include <unistd.h>
#include <assert.h>

#include "header.h"

// Function for reading commands from the client
void copydata(int from,int to) {
  char buf[1024];
  int amount;

  // Loop until no more data can be read from the source file descriptor
  while ((amount = read(from, buf, sizeof(buf))) > 0) {
    assert((write(to, buf, amount) == amount));
    handleCommand(buf, from);     // Invoke the handleCommand function on the read data
  }

  // Ensure that the read operation was successful
  assert(amount >= 0);
}