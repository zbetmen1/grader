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

#ifndef METHODS_SUPPORT_H
#define METHODS_SUPPORT_H

// STL headers
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <cstddef>
#include <algorithm>
#include <memory>

// Project headers
#include "object.hpp"
#include "any.hpp"

namespace dynamic
{
  // Typedefs
  class methods_support;
  using hash_methods = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;
  using methods_container = std::vector<std::pair<const char*, const char*>>;
  using safe_methods_support = std::unique_ptr<methods_support, object_dtor>;
  typedef void (*supported_method)(void*);
  
  class methods_support : public object
  {
    // Hash for methods and mutex to support threads
    static hash_methods m_hashMethods;
    static std::mutex m_lockHash;
    
    // Functions to work with methods cache
    static bool insert_methods_st(const std::string& className, const methods_container& container);
    static void erase_class_st(const std::string& className);
    static bool insert_methods_mt(const std::string& className, const methods_container& container);
    static void erase_class_mt(const std::string& className);
    
    std::vector<any> m_args;
    any m_result;
    std::size_t m_argsNum;
    
  protected:
    explicit methods_support();
    ~methods_support();
  public:
    virtual const char* name() const { return "methods_support"; }
    
    const any& result() const;
    any& result();
    const std::vector<any>& get_arguments() const;
    
    static const std::string& get_method_st(const std::string& className, const std::string& methodName);
    static const std::string& get_method_mt(const std::string& className, const std::string& methodName);
    
    template <typename ...ValueType>
    void arguments(ValueType ...args)
    {
      // Unpack elements and get it's number
      dynamic::any anyArray[] = {args...};
      std::size_t arraySize = sizeof(anyArray) / sizeof(dynamic::any);
      
      // Reserve vector memory and copy arguments
      m_args.reserve(arraySize);
      m_argsNum = arraySize;
      for (std::size_t i = 0U; i < m_argsNum; ++i)
        m_args.push_back(anyArray[i]);
    }
    
    friend class register_methods;
    friend class shared_lib;
  };
}

#endif // METHODS_SUPPORT_H
