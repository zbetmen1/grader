#include "dynamic/register_creators.hpp"
#include "dynamic/object.hpp"

#include <string>
#include <iostream>
#include <sstream>

using namespace std;

namespace dynamic  
{
  register_creators::register_creators(const char* className, dynamic::object_ctor ctorName, dynamic::object_dtor dtorName, const bool multithreaded) noexcept
  : m_className{className}, m_multithreaded{multithreaded}
  {
    if (multithreaded) 
    {
      if (!object::insert_creators_mt(className, ctorName, dtorName))
        throw dynamic_exception(string("Class named '") + className + "' already registered.\n");
    }
    else if (!object::insert_creators_st(className, ctorName, dtorName))
    {
      throw dynamic_exception(string("Class named '") + className + "' already registered.\n");
    }
  }
  
  register_creators::~register_creators() noexcept
  {
    if (m_multithreaded) object::erase_creators_mt(m_className.c_str());
    else object::erase_creators_st(m_className.c_str());
  }

}