#include "safe_process.hpp"
#include "process.hpp"

using namespace std;
using namespace grader;

int main()
{
  pipe_istream in; 
  pipe_ostream out;
  safe_process sfp("test_safe_proc", 
                   "/srv/chroot/trusty_X86_64/",
                   "/home/grader_jail_00",
                   999,
                   5000,
                   safe_process::unlimited,
                   safe_process::unlimited,
                   process::no_args,
                   &out, &in
                  );
  out << "1 2 3 4 5 6 7 8 9 0\n";
  out.close();
  sfp.wait();
  
  int n2;
  while (in >> n2)
  {
    cout << n2 << endl;
  }
}