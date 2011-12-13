#ifndef PRINT_H
#define PRINT_H
#include <pthread.h>
#include <stdio.h>

#include <stdarg.h>
int myprintf(char *fmt, ...)
{
  fprintf(stdout, "%2X", ((int)pthread_self())%256);
  va_list args;
  va_start(args, fmt);
  int r = vprintf(fmt, args);
  va_end(args);
  return r;
}

#define printf myprintf

#endif
