/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2014  Kocic Ognjen <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "shared_lib.hpp"
#include "object.hpp"

#include <iostream>
#include <cassert>
#include <unordered_map>

using namespace std;

namespace dynamic 
{
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

  safe_object shared_lib::make_object(const string& className) const
  {
    // Get constructor name for class name and check that the name is registered
    auto ctorName = object::constructor(className);
    if (ctorName.empty())
      return safe_object{nullptr, nullptr};
    
    // Check that this class is located in this shared library (other possibility is that constructor is misspelled during registration)
    object_ctor ctor = reinterpret_cast<object_ctor>(platform::fetch_sym_from_shared_lib(m_impl, ctorName.c_str()));
    if (nullptr == ctor)
      return safe_object{nullptr, nullptr};
    
    // Create new object
    object* obj = ctor();
    return safe_object{obj, obj->deleter()};
  }

  platform::shared_lib_c_fun_ptr 
  shared_lib::get_c_function(const string& functionName) const
  {
    return platform::fetch_sym_from_shared_lib(m_impl, functionName.c_str());
  }

  shared_lib::~shared_lib()
  {
    // Do nothing if library wasn't loaded, otherwise unload library
    if (nullptr != m_impl)
      platform::close_shared_lib(m_impl); // TODO: Figure out how to handle failure of dlclose?
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