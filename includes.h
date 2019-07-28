#ifndef INCLUDES_H
#define INCLUDES_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define CHILDREN 8

struct thread_args {
  char** board; // Board for the thread
  long move;     // The current move number
  int r;        // Row location
  int c;        // Column location
};

#endif
