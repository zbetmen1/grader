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

#ifndef GRADER_H
#define GRADER_H

// STL headers
#include <memory>
#include <cstddef>
#include <string>

// BOOST headers
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

namespace grader
{
  class grader
  {
  public:
    // Common types
    using path_t = boost::filesystem::path;
    
  private:
    path_t m_srcFile;
    path_t m_executable;
    std::size_t m_memoryLimit; // In bytes
    std::size_t m_timeLimit; // In milliseconds
  public:
    explicit grader(const path_t& srcFile, std::size_t memoryLimit, std::size_t timeLimit);
    virtual ~grader() {};
    
    virtual boost::optional<std::string> compile() = 0;
    virtual boost::optional<std::string> run_test(const path_t& testIn, const path_t& testOut) const = 0;
  public:
    static std::unique_ptr<grader> create_from_src(const path_t& srcFile, std::size_t memoryLimit, std::size_t timeLimit);
  };
}

#endif // GRADER_H
