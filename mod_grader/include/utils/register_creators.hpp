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

#ifndef REGISTER_CREATORS_H
#define REGISTER_CREATORS_H

// STL headers
#include <stdexcept>
#include <string>

// Project headers
#include "object.hpp"

namespace dynamic
{ 
  /**
   * @brief This class provides functionality of static constructor and static destructor for classes
   * that derive from reflection::object.
   * 
   */
  class register_creators
  {
    std::string m_className;
    bool m_multithreaded;
  public:
    explicit register_creators(const char* className, object_ctor ctorName, object_dtor dtorName, 
                                  const bool multithreaded = false) noexcept;
    ~register_creators() noexcept;
    
    // This class is not copyable, nor movable
    register_creators(const register_creators&) = delete;
    register_creators& operator=(const register_creators&) = delete;
    register_creators(register_creators&&) = delete;
    register_creators& operator=(register_creators&&) = delete;
  };
}

#endif // REGISTER_CREATORS_H
