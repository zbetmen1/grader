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

#include "any.hpp"

#include <utility>

namespace dynamic 
{
  any::any()
  : m_any{nullptr}
  {}
  
  any::any(const any& oth)
  : m_any{oth.m_any ? oth.m_any->clone() : nullptr}
  {}
  
  any::any(any&& moved)
  : m_any{moved.m_any}
  { moved.m_any = nullptr; }
    
  any::~any()
  {
    delete m_any;
  }
  
  any& any::swap(any& oth) noexcept
  {
    std::swap(m_any, oth.m_any);
    return *this;
  }
  
  any& any::operator=(const any& oth)
  {
    any{oth}.swap(*this);
    return *this;
  }
  
  any& any::operator=(any&& moved)
  {
    delete m_any;
    m_any = nullptr;
    return swap(moved);
  }
  
  bool any::empty() const
  {
    return !m_any;
  }
  
  void any::clear()
  {
    any{}.swap(*this);
  }
  
  inline const std::type_info& any::type() const
  {
    return m_any->type();
  }
  
  void swap(any& lhs, any& rhs) noexcept
  {
    lhs.swap(rhs);
  }
}