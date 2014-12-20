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

// Project headers
#include "subtest.hpp"
#include "object.hpp"
#include "register_creators.hpp"
#include "task.hpp"

// STL headers
#include <string>
#include <vector>

// Poco headers
#include <Poco/Process.h>

namespace Poco
{
  class Pipe;
  class PipeInputStream;
}

namespace grader
{ 
  class grader_base: public dynamic::object
  {
    task* m_task;
    std::string m_dirPath;
    std::string m_executablePath;
    std::string m_sourcePath;
  public:
    // Grader is DefaultConstructible
    grader_base();
    
    // Grader base is copyable
    grader_base(const grader_base& oth) = default;
    grader_base& operator=(const grader_base& oth) = default;
    
    // Grader base is movable
    grader_base(grader_base&& oth) = default;
    grader_base& operator=(grader_base&& oth) = default;
    
    // Grader is base class so destructor is virtual and we must have initialization method (cause of shared library loading)
    virtual ~grader_base();
    void initialize(task* t);
    
    // Informations about particular grader
    virtual const std::vector<const char*>& extensions() const = 0;
    virtual const char* language() const = 0;
    
    // API
    virtual bool compile(std::string& compileErr) const;
    virtual bool run_test(const test& t) const;
    
    // Implementing object's virtual function so graders can be created from shared libraries in runtime
    virtual const char* name() const { std::string gr("grader_"); return (gr + language()).c_str(); }
  protected:
    // Each grader needs to provide these informations so base class can compile sources and run executables
    virtual bool is_compilable() const = 0;
    virtual const char* compiler() const = 0;
    virtual void compiler_flags(std::string& flags) const = 0;
    virtual const char* compiler_filename_flag() const = 0;
    virtual const char* compiler_stdin_flag() const { return ""; }
    virtual bool should_write_src_file() const = 0;
    virtual const char* executable_extension() const { return ""; };
    
    // Different languages require different ways to start process (for example Java uses 'java StartMe.class')
    virtual Poco::ProcessHandle start_executable_process(const std::string& executable, const std::vector< std::string >& args, 
                                                         const std::string& workingDir, Poco::Pipe* toBinaries, Poco::Pipe* fromBinaries) const;
    
  private:
    // Utilities
    std::string strip_extension(const std::string& fileName) const;
    std::string get_extension(const std::string& fileName) const;
    std::string dir_path() const;
    std::string source_path() const;
    std::string binaries_path() const;
    void write_to_disk(const std::string& path, const std::string& content) const;
    Poco::ProcessHandle run_compile(std::string& flags, Poco::Pipe& errPipe) const;
    
    // Run test cases
    bool run_test_std_std(const subtest& in, const subtest& out, const std::string& executable, 
                                   Poco::Pipe& toBinaries, Poco::Pipe& fromBinaries) const;
    bool run_test_cmd_std(const subtest& in, const subtest& out, 
                                   const std::string& executable, Poco::Pipe& fromBinaries) const;
    bool run_test_file_std(const subtest& in, const subtest& out, 
                                   const std::string& executable, Poco::Pipe& fromBinaries) const;
    bool run_test_std_file(const subtest& in, const subtest& out, const std::string& executable, 
                                   Poco::Pipe& toBinaries) const;
    bool run_test_cmd_file(const subtest& in, const subtest& out, const std::string& executable) const;
    
    bool run_test_file_file(const subtest& in, const subtest& out, const std::string& executable) const;
    
    std::vector<std::string> create_file_input(const subtest& in) const;
    
    bool evaluate_output_stdin(Poco::PipeInputStream& fromBinariesStream, const grader::subtest& out, 
                               const Poco::ProcessHandle& ph) const;
    bool evaluate_output_file(const std::string& absolutePath, const subtest& out, const Poco::ProcessHandle& ph) const;
                                   
  };
}

#endif // GRADER_BASE_H
