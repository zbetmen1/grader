#include <string>

#include "process.hpp"

using namespace grader;
using namespace std;

int main()
{
  pipe_ostream out;
  pipe_istream in;
  process p{"/home/zbetmen/myTest", process::no_args, &out, &in};
  out << 1  << ' '
      << 1  << ' '
      << 2  << ' '
      << 3  << ' '
      << 5  << ' '
      << 8  << ' '
      << 13 << ' '
      << 0  << ' ';
  out.close();
  p.wait();
  
  int n2;
  while (in >> n2)
    cout << "n^2=" << n2 << endl;
}