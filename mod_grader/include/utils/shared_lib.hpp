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

#ifndef SHARED_LIB_H
#define SHARED_LIB_H

// STL headers
#include <string>
#include <stdexcept>
#include <type_traits>

// Project headers
#include "object.hpp"
#include "platform_specific.hpp"

namespace dynamic
{ 
  
  class shared_lib_load_failed: public std::runtime_error
  {
  public:
    explicit shared_lib_load_failed(const char* arg)
    : std::runtime_error{arg}
    {}
  };
  
  // Forward declaration of object to minimize compilation dependencies
  class object;
  
  enum class shared_lib_mode: int
  {
    LAZY = RTLD_LAZY,
    NOW = RTLD_NOW
  };
  
  class shared_lib
  {
    platform::shared_lib_impl m_impl;
    
  public:  
    shared_lib(const std::string& p, shared_lib_mode flag = shared_lib_mode::LAZY);
    ~shared_lib();
    
    platform::shared_lib_c_fun_ptr 
    get_c_function(const std::string& functionName) const;
    
    // Shared library shouldn't be copied
    shared_lib(const shared_lib&) = delete;
    shared_lib& operator=(const shared_lib&) = delete;
    
    // Shared library should be movable
    shared_lib(shared_lib&& moved);
    shared_lib& operator=(shared_lib&& moved);
    
    template <typename Derived>
    std::unique_ptr<Derived, object_dtor> make_object(const std::string& className)
    {
      // Get constructor pointer class name and check that the name is registered
      auto ctor = object::constructor(className);
      if (!ctor)
        return std::unique_ptr<Derived, object_dtor>{nullptr, nullptr};
      
      // Create new object
      Derived* dobj = static_cast<Derived*>((*ctor)());
      return std::unique_ptr<Derived, object_dtor>{dobj, object::destructor(className)};
    }
  };
}

#endif // SHARED_LIB_H
