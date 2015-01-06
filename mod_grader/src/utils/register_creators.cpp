#include "register_creators.hpp"
#include "object.hpp"
#include "grader_log.hpp"

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
      {
        stringstream logmsg;
        logmsg << "Class named '" << className << "' already registered.";
        LOG(logmsg.str(), grader::ERROR);
      }
    }
    else if (!object::insert_creators_st(className, ctorName, dtorName))
    {
      stringstream logmsg;
      logmsg << "Class named '" << className << "' already registered.";
      LOG(logmsg.str(), grader::ERROR);
    }
  }
  
  register_creators::~register_creators() noexcept
  {
    if (m_multithreaded) object::erase_creators_mt(m_className.c_str());
    else object::erase_creators_st(m_className.c_str());
  }

}