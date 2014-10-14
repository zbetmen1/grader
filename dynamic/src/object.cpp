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

#include "object.hpp"
#include <vector>

using namespace std;

namespace dynamic 
{ 
  hash_constructors object::m_hashCtorName;
  hash_methods object::m_hashClassMethods;
  
  object::object(object_dtor deleter)
  : m_deleter{deleter}
  {
  }
  
  std::string object::constructor(const std::string& className)
  {
    return m_hashCtorName[className];
  }
  
  // NOTE: This destructor MUST NOT be made inline cause of vtable lookup! It would break runtime destruction.
  object::~object() {}
  
  std::string object::get_c_wrapper_name(const std::string& className, const std::string& cppFunctionName)
  {
    // Check if class was registered with any method
    vector<function_pair_names> v = m_hashClassMethods[className];
    if (v.empty())
      return "";
    
    // Iterate through methods
    for (const auto& p : v)
      if (p.cpp_function == cppFunctionName)
        return p.c_function;
    
    // We haven't found registered C++ function with name 'cppFunctionName'
    return "";
  }
}