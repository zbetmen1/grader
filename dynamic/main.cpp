#include <iostream>
#include <memory>
#include <vector>

#include "shared_lib.hpp"
#include "object.hpp"

using namespace std;
using namespace dynamic;

int main(int , char **) 
{
  try 
  {
    shared_lib testLib = shared_lib{"/home/zbetmen/Documents/programming/grader/dynamic/lib/libtestdynamic.so", shared_lib_mode::LAZY};
    safe_object uniqueTestObject = testLib.make_object("test_object");
    
    // Test invoke function for void
    invoke_function<void>(testLib, uniqueTestObject.get(), "test_method_void");
    
    // Test fill vector
    std::size_t n = 10;
    vector<int> v(n);
    invoke_function<void>(testLib, uniqueTestObject.get(), "test_fill_vector", &v.front(), n); 
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  
  return 0;
}
