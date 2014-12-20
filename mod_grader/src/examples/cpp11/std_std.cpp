#include <iostream>
#include <iterator>
#include <numeric>

using namespace std;

int main()
{
  cout << accumulate(istream_iterator<long long>(cin), istream_iterator<long long>(), 0LL) << endl;
}