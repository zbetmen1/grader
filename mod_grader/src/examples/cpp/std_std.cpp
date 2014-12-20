#include <iostream>

using namespace std;

int main()
{
  unsigned n;
  cin >> n;
  long long sum = 0;
  for (unsigned i = 0; i < n; ++i)
  {
    int current;
    cin >> current;
    sum += current;
  }
  cout << sum << endl;
}