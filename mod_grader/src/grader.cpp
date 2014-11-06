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

// Project headers
#include "grader.hpp"

// STL headers
#include <stdexcept>

using namespace std;

namespace grader
{
  grader::grader::grader(const grader::grader::path_t& srcFile, std::size_t memoryLimit, std::size_t timeLimit)
  : m_srcFile(srcFile), m_memoryLimit(memoryLimit), m_timeLimit(timeLimit)
  {
  }

  std::unique_ptr<grader> grader::create_from_src(const grader::path_t& /*srcFile*/, 
                                                  std::size_t /*memoryLimit*/, std::size_t /*timeLimit*/)
  {
    return unique_ptr<grader>();
  }
}