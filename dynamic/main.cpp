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
    shared_lib testLib = shared_lib{"/home/zbetmen/Documents/programming/grader/test_dynamic/build/libtestdynamic.so", shared_lib_mode::LAZY};
    safe_object uniqueTestObject = testLib.make_object("test_object");
    cout << "Object name is: " << uniqueTestObject->name() << endl;
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  
  return 0;
}
