#include "dynamic/shared_lib.hpp"
#include "dynamic/object.hpp"

#include <iostream>
#include <cassert>
#include <unordered_map>

using namespace std;

namespace dynamic 
{
  shared_lib::shared_lib()
  {}
  
  shared_lib::shared_lib(const string& p, dynamic::shared_lib_mode flag)
  { 
    // Resolve path, resolve mode integer value and try to open shared library
    typedef void (*free_declaration_ptr)(void*);
    unique_ptr<char, free_declaration_ptr> realPath{platform::real_path(p.c_str()), &::free};
    if (!realPath)
    {
      throw shared_lib_load_failed{platform::error_from_os_code()};
    }
    auto realFlag = static_cast<int>(flag);
    m_impl = platform::open_shared_lib(realPath.get(), realFlag);
    
    // Check that everything went well
    if (nullptr == m_impl)
    { 
      // Construct initial error string
      string msg{"Failed to load library on path '"};
      msg += realPath.get();
      msg += "'!\n";
      
      // Append info from shared_lib_error() if there's one
      auto dlerrorMsg = platform::shared_lib_error();
      if (nullptr != dlerrorMsg)
        msg += dlerrorMsg;
      
      // Throw exception saying what happened
      throw shared_lib_load_failed{msg.c_str()};
    }
  }

  platform::shared_lib_c_fun_ptr 
  shared_lib::get_c_symbol(const string& functionName) const
  {
    return platform::fetch_sym_from_shared_lib(m_impl, functionName.c_str());
  }

  shared_lib::~shared_lib()
  {
    // Do nothing if library wasn't loaded, otherwise unload library
    if (nullptr != m_impl)
      platform::close_shared_lib(m_impl); // TODO: Figure out how to handle failure of close_shared_lib?
  }

  shared_lib::shared_lib(shared_lib&& moved)
  : m_impl{moved.m_impl}
  {
    moved.m_impl = nullptr;
  }
  
  shared_lib& shared_lib::operator=(shared_lib&& moved)
  {
    // Close this library, move pointer from 'moved' and set it's pointer to nullptr
    platform::close_shared_lib(m_impl);
    m_impl = moved.m_impl;
    moved.m_impl = nullptr;
    return *this;
  }
}