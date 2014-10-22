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

#ifndef TEST_METHODS_SUPPORT_H
#define TEST_METHODS_SUPPORT_H

// STL headers
#include <vector>
#include <functional>

// Project headers
#include "methods_support.hpp"
#include "any.hpp"
#include "register_creators.hpp"
#include "register_methods.hpp"

class test_methods_support: public dynamic::methods_support
{
public:
    explicit test_methods_support();
    virtual const char* name() const;
    std::string int_to_str(int x) const;
    int min_v_int(std::reference_wrapper<std::vector<int>> v) const;
};

REGISTER_DYNAMIC_METHODS_ST(test_methods_support, int_to_str, min_v_int)
WRAP_DYNAMIC_METHOD(test_methods_support, int_to_str, int)
WRAP_DYNAMIC_METHOD(test_methods_support, min_v_int, std::reference_wrapper<std::vector<int>>)

#endif // TEST_METHODS_SUPPORT_H
