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

#include "object.hpp"
#include <vector>

using namespace std;

namespace dynamic 
{ 
  hash_creators object::m_hashCreatorsName;
  mutex object::m_lockHashes;
  
  void object::insert_creators_st(const char* className, object_ctor ctorName, object_dtor dtorName)
  {
    m_hashCreatorsName[className] = make_pair(ctorName, dtorName);
  }

  void object::insert_creators_mt(const char* className, object_ctor ctorName, object_dtor dtorName)
  {
    lock_guard<mutex> lockHash{m_lockHashes};
    insert_creators_st(className, ctorName, dtorName);
  }
  
  void object::erase_creators_st(const char* className)
  {
    m_hashCreatorsName.erase(className);
  }

  void object::erase_creators_mt(const char* className)
  {
    lock_guard<mutex> lockHash{m_lockHashes};
    erase_creators_st(className);
  }
  
  object_dtor object::destructor(const string& className)
  {
    return m_hashCreatorsName[className].second;
  }
  
  object_ctor object::constructor(const std::string& className)
  {
    return m_hashCreatorsName[className].first;
  }
  
  object::object()
  { }
  
  object::~object() {}
  
}