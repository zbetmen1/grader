#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  long long sum = -1;
  if (2 >= argc)
  {
    unsigned n;
    scanf("%u", &n);
    sum = 0;
    int current = 0;
    for (unsigned i = 0; i < n; ++i)
    {
      scanf("%i", &current);
      sum += current;
    }
  }
  
  FILE* out = fopen(argv[1], "w");
  fprintf(out, "%lld\n", sum);
  return 0;
}