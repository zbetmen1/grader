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

// Project headers
#include "methods_support.hpp"

using namespace std;

namespace dynamic
{
  hash_methods methods_support::m_hashMethods;
  mutex methods_support::m_lockHash;
  
  const string& methods_support::get_method_st(const string& className, const string& methodName)
  {
    return m_hashMethods[className][methodName];
  }

  bool methods_support::insert_methods_st(const string& className, const dynamic::methods_container& container)
  {
    if (m_hashMethods.cend() != m_hashMethods.find(className))
      return false;
    
    m_hashMethods[className] = unordered_map<std::string, std::string>{};
    for (const auto& p : container)
      m_hashMethods[className].insert(p);
    return true;
  }
  
  void methods_support::erase_class_st(const string& className)
  {
    m_hashMethods.erase(className);
  }
  
  const string& methods_support::get_method_mt(const string& className, const string& methodName)
  {
    lock_guard<mutex> lockHash{m_lockHash};
    return get_method_st(className, methodName);
  }

  bool methods_support::insert_methods_mt(const string& className, const dynamic::methods_container& container)
  {
    lock_guard<mutex> lockHash{m_lockHash};
    return insert_methods_st(className, container);
  }
  
  void methods_support::erase_class_mt(const string& className)
  {
    lock_guard<mutex> lockHash{m_lockHash};
    erase_class_st(className);
  }
  
  methods_support::methods_support()
  : object{}
  {
  }
  
  methods_support::~methods_support()
  {
  }

  const any& methods_support::result() const
  {
    return m_result;
  }
  
  any& methods_support::result()
  {
    return m_result;
  }

  const std::vector< any >& methods_support::get_arguments() const
  {
    return m_args;
  }

}
