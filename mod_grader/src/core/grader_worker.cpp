#include "safe_process.hpp"

using namespace std;
using namespace grader;

int main()
{
  pipe_istream in; 
  pipe_ostream out;
  safe_process sfp("/tmp/grader_sandbox_testing/testdir/myTest", 
                   "/tmp/grader_sandbox_testing/",
                   "/tmp/grader_sandbox_testing/testdir/",
                   999,
                   1000,
                   1 << 24,
                   5,
                   process::no_args,
                   &in, &out
                  );
}