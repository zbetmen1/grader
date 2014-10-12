#include <iostream>
#include <memory>

#include "shared_lib.hpp"
#include "object.hpp"
#include "test_object.hpp"

using namespace std;
using namespace reflection;

int main(int , char **) 
{
  try 
  {
    shared_lib testLib = shared_lib{"/home/zbetmen/Documents/programming/grader/reflection/lib/libtestreflection.so", shared_lib_mode::LAZY};
    auto safeTestObject0 = get_derived_safe<test_object>(testLib.make_object("test_object"));
    auto safeTestObject1 = get_derived_safe<test_object>(testLib.make_object("test_object"));
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  
  return 0;
}
