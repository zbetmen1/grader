#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/inotify.h>

int main()
{
  char* memblk = calloc(67108864, 1); // 64MB
  int maxMem = 0;
  int fd = inotify_init();
  
  free(memblk);
  pause();
  return 0;
}