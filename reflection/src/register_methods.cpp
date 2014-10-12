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

#include "register_methods.hpp"
#include <object.hpp>

using namespace std;

namespace reflection 
{
  register_methods::register_methods(const std::string& className, const std::vector< function_pair_names >& methods)
  : m_className{className}
  {
    if (object::m_hashClassMethods.cend() != object::m_hashClassMethods.find(className))
    {
      string msg{"Class with name '"};
      msg += className;
      msg += "' is already registered and there can't be two classes of same name registered!";
      throw class_already_exists{msg.c_str()};
    }
    else
    {
      object::m_hashClassMethods[className] = methods;
    }
  }

  register_methods::~register_methods()
  {
    object::m_hashClassMethods.erase(m_className);
  }
}