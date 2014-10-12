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
    shared_lib testLib{"/home/zbetmen/Documents/programming/grader/reflection/lib/libtestreflection.so", shared_lib_mode::LAZY};
    
    object* obj = testLib.make_object("test_object");
    test_object* tobj = dynamic_cast<test_object*>(obj);
    unique_ptr<test_object, object_dtor> uniqueObj{tobj, tobj->deleter()};
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  return 0;
}
