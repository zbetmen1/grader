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

#ifndef OBJECT_H
#define OBJECT_H

// STL headers
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

// Project headers
#include "dynamic_macros.hpp"

namespace dynamic
{
  // Typedefs
  class object;
  typedef void (*object_dtor)(void*);
  typedef void* (*object_ctor)();
  using safe_object = std::unique_ptr<object, object_dtor>;
  using hash_creators = std::unordered_map<std::string, std::pair<object_ctor, object_dtor>>;
  
  class object
  { 
    // Hash for constructors and mutex to protect it in case when libraries are loaded in different threads
    static std::mutex m_lockHashes;
    static hash_creators m_hashCreatorsName;
    
    // Functions that register and unregister class creators in single thread loading scenario (_st) 
    // and multi-threaded scenario (_mt), this extensions semantic applies to REGISTER_DYNAMIC* macros
    static bool insert_creators_st(const char* className, object_ctor ctorName, object_dtor dtorName);
    static bool insert_creators_mt(const char* className, object_ctor ctorName, object_dtor dtorName);
    static void erase_creators_st(const char* className);
    static void erase_creators_mt(const char* className);
    
    // Functions that return pointers to class creators
    static object_ctor constructor(const std::string& className);
    static object_dtor destructor(const std::string& className);
  protected:
    explicit object();
    virtual ~object() = 0;
  public:
    virtual const char* name() const { return "object"; }
    
    friend class register_creators;
    friend class shared_lib;
  };
}
  
#endif // OBJECT_H
