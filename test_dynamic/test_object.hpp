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

#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include "object.hpp"
#include "register_constructor.hpp"
#include "register_methods.hpp"

#include <memory>

class test_object: public dynamic::object
{
  DYNAMIC_OBJECT
public:
  test_object(dynamic::object_dtor deleter);
  virtual std::string name() const { return "test_object"; }
};

extern "C" 
void* create_test_object();

extern "C"
void destroy_test_object(void* deletedObject);

#endif // TEST_OBJECT_H
