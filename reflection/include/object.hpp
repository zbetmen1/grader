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

// Project headers
#include "reflection_types.hpp"

namespace reflection
{
  /**
   * @brief This class represents base class for all objects that can be constructed dynamically 
   * from shared library without previous linking in compile time. 
   * @details This class is made pure virtual as nobody should create instance of this particular class. 
   * It has following purposes:
   *   1) returning any derived class from shared library through pointer object*
   *   2) provides mechanism for safe destruction of derived objects
   *   3) manages names of C functions, that are stored hash data structure, that are used to create classes
   *      that derive from 'object'.
   */
  class object
  { 
    static std::unordered_map<std::string, std::string> m_hashCtorName;
    
    object_dtor m_deleter;
  protected:
    object(object_dtor deleter);
    virtual ~object() = 0;
  public:
    inline const object_dtor deleter() const { return static_cast<const object_dtor>(m_deleter); }
    
    static std::string constructor(const std::string& className);
    friend class register_constructor;
  };
  
  template <typename Derived>
  std::unique_ptr<Derived, object_dtor> get_derived_safe(object* obj)
  {
    Derived* derived = dynamic_cast<Derived*>(obj);
    if (nullptr == derived)
      return std::unique_ptr<Derived, object_dtor>{nullptr, nullptr};
    else
      return std::unique_ptr<Derived, object_dtor>{derived, derived->deleter()};
  }
}

#endif // OBJECT_H
