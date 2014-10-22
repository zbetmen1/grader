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
#include "methods_support.hpp"

namespace dynamic
{
  register_methods::register_methods(const char* className, const methods_container& methods, const bool multithreaded) noexcept
  : m_className{className}, m_multithreaded{multithreaded}
  { 
    try
    {
      if (multithreaded)
      {
        if (!methods_support::insert_methods_mt(className, methods))
        {
          // TODO: Log error!
        }
      }
      else if (!methods_support::insert_methods_st(className, methods))
      {
        // TODO: Log error!
      }
    } catch (const std::exception& e)
    {
      // TODO: Log error!
    } catch (...)
    {
      // TODO: Log error!
    }
  }
 
  register_methods::~register_methods() noexcept
  {
    try
    {
      if (m_multithreaded) methods_support::erase_class_mt(m_className);
      else methods_support::erase_class_st(m_className);
    } catch (const std::exception& e)
    {
      // TODO: Log error!
    } catch (...)
    {
      // TODO: Log error!
    }
  }
}