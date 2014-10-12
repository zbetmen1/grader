#include <iostream>
#include <memory>
#include <vector>

#include <test_object.hpp>
#include "shared_lib.hpp"
#include "object.hpp"

using namespace std;
using namespace reflection;

int main(int , char **) 
{
  try 
  {
    shared_lib testLib = shared_lib{"/home/zbetmen/Documents/programming/grader/reflection/lib/libtestreflection.so", shared_lib_mode::LAZY};
    
    auto uniqueTestObject = get_derived_safe<test_object>(testLib.make_object("test_object"));
    
    // Test invoke function for void
    invoke_function<void>(testLib, uniqueTestObject.get(), "test_method_void");
    
    // Test invoke function for floating point
    float testReal = invoke_function<float>(testLib, uniqueTestObject.get(), "test_method_real", 5, 2);
    cerr << "test_method_real returned: " << testReal << endl;
    
    // Test fill vector
    std::size_t n = 10;
    vector<int> v(n);
    invoke_function<void>(testLib, uniqueTestObject.get(), "test_fill_vector", &v.front(), n);
    for (auto j : v)
      cout << j << endl; 
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  
  return 0;
}
