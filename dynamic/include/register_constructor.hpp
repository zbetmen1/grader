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

#ifndef REGISTER_YOURSELF_H
#define REGISTER_YOURSELF_H

#include <stdexcept>
#include <string>

namespace dynamic
{ 
  /**
   * @brief This class provides functionality of static constructor and static destructor for classes
   * that derive from reflection::object.
   * 
   */
  class register_constructor
  {
    std::string m_className;
  public:
    explicit register_constructor(const char* className, const char* ctorName, const bool multithreaded = false) noexcept;
    ~register_constructor() noexcept;
    
    // This class is not copyable, nor movable
    register_constructor(const register_constructor&) = delete;
    register_constructor& operator=(const register_constructor&) = delete;
    register_constructor(register_constructor&&) = delete;
    register_constructor& operator=(register_constructor&&) = delete;
  };
}

#endif // REGISTER_YOURSELF_H
