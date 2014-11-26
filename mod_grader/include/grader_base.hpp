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

#ifndef GRADER_BASE_H
#define GRADER_BASE_H

// STL headers
#include <string>
#include <vector>

// Poco headers
#include <Poco/Process.h>

namespace grader
{
  class test;
  
  class grader_base
  {
    std::string m_fileName;
    std::string m_fileContent;
    std::string m_uuid;
  public:
    grader_base(const std::string& fileName, const std::string& fileContent, const std::string& uuid);
    
    // Grader base is copyable
    grader_base(const grader_base& oth) = default;
    grader_base& operator=(const grader_base& oth) = default;
    
    // Grader base is movable
    grader_base(grader_base&& oth) = default;
    grader_base& operator=(grader_base&& oth) = default;
    
    // Grader is base class so destructor is virtual
    virtual ~grader_base();
    
    // Informations about particular grader
    virtual const std::vector<const char*>& extensions() const = 0;
    virtual const char* language() const = 0;
    
    // Each grader needs to provide these informations so base class can compile sources
    virtual bool is_compilable() const = 0;
    virtual std::string compiler() const = 0;
    virtual void compiler_flags(std::vector<std::string>& flags) const = 0;
    virtual std::string compiler_filename_flag() const = 0;
    virtual bool is_compiling_from_stdin() const = 0;
    
    // API
    bool compile(std::string& compileErr) const;
    virtual bool run_test(const test& t) const = 0;
    
  protected:
    const std::string& fileName() const;
    
  private:
    // Utilities
    std::string strip_extension(const std::string& fileName) const;
    std::string get_extension(const std::string& fileName) const;
    std::string dir_path() const;
    std::string source_path() const;
    std::string binaries_path() const;
    void write_src() const;
    Poco::ProcessHandle run_compile(const std::string& compilerCmd, const std::vector< std::string >& flags, Poco::Pipe& errPipe) const;
  };
}
#endif // GRADER_BASE_H
