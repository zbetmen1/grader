#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  // We exit here if we don't have path as argument or can't open file
  if (2 < argc)
  {
    printf("-1\n");
    exit(EXIT_SUCCESS);
  }
  
  // Open file
  char* path = argv[1];
  FILE* in = fopen(path, "r");
  if (NULL == in)
  {
    printf("%s\npath: %s\n current dir: %s\n", strerror(errno), argv[1], get_current_dir_name());
    exit(EXIT_SUCCESS);
  }
  
  // Sum numbers
  unsigned n;
  fscanf(in, "%u", &n);
  long long sum = 0;
  for (unsigned i = 0; i < n; ++i)
  {
    int current;
    fscanf(in, "%d", &current);
    sum += current;
  }
  printf("%lld\n", sum);
  exit(EXIT_SUCCESS);
}