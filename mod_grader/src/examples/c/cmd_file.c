#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  char* path = argv[1];
  long long sum = 0;
  for (unsigned i = 2; i < argc; ++i)
  {
    sum += atoi(argv[i]);
  }
  fprintf(fopen(path, "w"),"%lld\n", sum);
  return 0;
}