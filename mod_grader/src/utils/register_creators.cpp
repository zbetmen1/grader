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

#include "register_creators.hpp"
#include "object.hpp"

#include <string>
#include <iostream>

using namespace std;

namespace dynamic  
{
  register_creators::register_creators(const char* className, dynamic::object_ctor ctorName, dynamic::object_dtor dtorName, const bool multithreaded) noexcept
  : m_className{className}, m_multithreaded{multithreaded}
  {
    try
    {
      if (multithreaded) 
      {
        if (!object::insert_creators_mt(className, ctorName, dtorName))
        {
          // TODO: Log error!
        }
      }
      else if (!object::insert_creators_st(className, ctorName, dtorName))
      {
        // TODO: Log error!
      }
    } catch(const std::exception& e) {
      // TODO: Log error!
    } catch(...)
    {
      // TODO: Log error!
    }
  }
  
  register_creators::~register_creators() noexcept
  {
    try 
    {
      if (m_multithreaded) object::erase_creators_mt(m_className.c_str());
      else object::erase_creators_st(m_className.c_str());
    } 
    catch (const exception& e) 
    {
      // TODO: Log error!
    } catch (...)
    {
      // TODO: Log error!
    }
  }

}