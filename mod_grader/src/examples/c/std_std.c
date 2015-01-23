#include <stdio.h>
/*
 * This example outputs sum of all numbers given from input and outputs sum to output.
 * Input and output types are present in file name.
 * */

int main()
{
  unsigned n;
  scanf("%u", &n);
  long sum = 0;
  int current = 0;
  for (unsigned i = 0; i < n; ++i)
  {
    scanf("%i", &current);
    sum += current;
  }
  printf("%ld \n", sum);
  return 0;
}