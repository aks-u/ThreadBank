#include "header.h"

extern Desk deskArray[NUM_DESKS];

// Function to find the desk with the shortest queue
Desk* shortestQueue() {
  int minSize = deskArray[0].queueSize;
  Desk* minDesk = &deskArray[0];

  // Iterate through the array of desks to find the one with the minimum queue size
  for (int i = 1; i < NUM_DESKS; i++) {
    if (deskArray[i].queueSize < minSize) {
      minSize = deskArray[i].queueSize;
      minDesk = &deskArray[i];
    }
  }

  // Return a pointer to the desk with the shortest queue size
  return minDesk;
}