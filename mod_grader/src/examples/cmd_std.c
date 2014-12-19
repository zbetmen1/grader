#include <stdio.h>
#include <stdlib.h>

/*
 * This example outputs sum of all numbers given from input and outputs sum to output.
 * Input and output types are present in file name.
 * */

int main(int argc, char** argv)
{
  // Check number of args
  if (2 > argc)
  {
    printf("-1\n");
    exit(EXIT_SUCCESS);
  }
  
  // Sum elements
  long long sum = 0;
  for (int i = 1; i < argc; ++i)
  {
    sum += atoi(argv[i]);
  }
  printf("%lld\n", sum);
  exit(EXIT_SUCCESS);
}