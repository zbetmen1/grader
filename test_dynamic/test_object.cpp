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

#include "test_object.hpp"

#include <string>
#include <iostream>
#include <cstdarg>

using namespace dynamic;
using namespace std;

REGISTER_OBJECT(test_object, create_test_object)

test_object::test_object(dynamic::object_dtor deleter)
: object{deleter}
{}

void* create_test_object()
{
  test_object* obj = new test_object{&destroy_test_object};
  cerr << "test_object created!" << endl;
  return static_cast<void*>(obj);
}

void destroy_test_object(void* deletedObject)
{
  test_object* obj = static_cast<test_object*>(deletedObject);
  delete obj;
  cerr << "test_object deleted!" << endl;
}