#include <string>

#include "process.hpp"

using namespace grader;
using namespace std;

int main()
{
  process p("/home/zbetmen/myTest", process::restrictions_array{{process::cpu_time, 2000}});
  p.wait();
}