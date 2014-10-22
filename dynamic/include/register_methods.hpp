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

#ifndef REGISTER_METHODS_H
#define REGISTER_METHODS_H

// STL headers
#include <vector>
#include <map>
#include <string>

namespace dynamic
{
  using methods_container = std::vector<std::pair<const char*, const char*>>;
  
  class register_methods
  {
    std::string m_className;
    bool m_multithreaded;
  public:
    explicit register_methods(const char* className, const methods_container& methods, 
                              const bool multithreaded = false) noexcept;
    ~register_methods() noexcept;
    
    // Not copyable and not movable
    register_methods(const register_methods&) = delete;
    register_methods& operator=(const register_methods&) = delete;
    register_methods(register_methods&&) = delete;
    register_methods& operator=(register_methods&&) = delete;
  };
}

#endif // REGISTER_METHODS_H
