#include <iostream>
#include <memory>
#include <vector>
#include <iterator>

#include "shared_lib.hpp"
#include "object.hpp"
#include "any.hpp"
#include "methods_support.hpp"

using namespace std;
using namespace dynamic;

int main(int , char **) 
{
  try 
  {
    // Open shared library and fetch object
    shared_lib testLib = shared_lib{"../../test_dynamic/build/libtestdynamic.so", shared_lib_mode::LAZY};
    auto uniqueTestObject = testLib.make_object<methods_support>("test_methods_support");
    cout << "Object name is: " << uniqueTestObject->name() << endl;
    
    // Sort vector using non-virtual member function from runtime-loaded shared library class
    // Also return minimum (test return values)
    vector<int> v = { 1, 17, 0, -1, 5, 64};
    auto res = invoke_method<int>(testLib, uniqueTestObject, "min_v_int", std::ref(v)); // References must be wrapped!
    copy(v.cbegin(), v.cend(), ostream_iterator<int>(cout, " "));
    cout << endl << "Minimum is: " << res << endl;
  } 
  catch (const shared_lib_load_failed& e) 
  {
    cerr << e.what() << endl;
  }
  return 0;
}
