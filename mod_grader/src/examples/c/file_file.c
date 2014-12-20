#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  FILE* in = fopen(argv[1], "r");
  FILE* out = fopen(argv[2], "w");
  long long sum = 0;
  int current;
  while (EOF != fscanf(in, "%d", &current))
    sum += current;
  fprintf(out, "%lld\n", sum);
  return 0;
}