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
#include "grader_base.hpp"
#include "configuration.hpp"

// STL headers
#include <utility>

// BOOST headers
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>

// Poco headers
#include <Poco/Process.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

using namespace std;

namespace grader
{
  grader_base::grader_base(const std::string& fileName, const std::string& fileContent, const std::string& uuid)
  : m_fileName(fileName), m_fileContent(fileContent), m_uuid(uuid)
  {
    boost::filesystem::create_directory(dir_path());
  }
  
  grader_base::~grader_base()
  {
    boost::filesystem::remove_all(dir_path());
  }

  string grader_base::dir_path() const
  {
    auto dpath = configuration::instance().get(configuration::BASE_DIR)->second + "/" + m_uuid;
    return move(dpath);
  }
  
  string grader_base::source_path() const
  {
    return dir_path() + "/" + m_fileName;
  }
  
  string grader_base::binaries_path() const
  {
    return dir_path() + "/" + strip_extension(m_fileName);
  }

  string grader_base::strip_extension(const string& fileName) const
  {
    auto pointPos = fileName.find_last_of('.');
    auto fileNameLen = fileName.length();
    return fileName.substr(0, fileNameLen - pointPos);
  }

  string grader_base::get_extension(const string& fileName) const
  {
    auto pointPos = fileName.find_last_of('.');
    return fileName.substr(pointPos + 1);
  }
  
  void grader_base::write_src() const
  {
    boost::iostreams::mapped_file_params params;
    params.path = source_path();
    params.new_file_size = m_fileContent.length();
    params.flags = boost::iostreams::mapped_file::mapmode::readwrite;
    boost::iostreams::mapped_file mf;
    mf.open(params);
    copy(m_fileContent.cbegin(), m_fileContent.cend(), mf.data());
  }
  
  Poco::ProcessHandle grader_base::run_compile(const string& compilerCmd, const vector<string>& flags, 
                                               Poco::Pipe& errPipe) const
  {
    // Launch compiler command
    if (is_compiling_from_stdin())
    {
      // Write file content to stdin to give compiler
      Poco::Pipe stdinPipe;
      Poco::PipeOutputStream stdinPipeStream(stdinPipe);
      stdinPipeStream << m_fileContent;
      return Poco::Process::launch(compilerCmd, flags, &stdinPipe, nullptr, &errPipe);
    }
    else 
    {
      write_src();
      return Poco::Process::launch(compilerCmd, flags, nullptr, nullptr, &errPipe);
    }
  }
  
  bool grader_base::compile(string& compileErr) const
  {
    // Check if we need to compile at all
    if (!is_compilable())
      return true;
    
    // Set up compiler command and it's arguments
    auto compilerCmd = compiler();
    auto outputPath = binaries_path();
    vector<string> flags;
    compiler_flags(flags);
    flags.push_back(compiler_filename_flag() + " " + outputPath);
    
    // Launch compiler
    Poco::Pipe errPipe;
    auto ph = run_compile(compilerCmd, flags, errPipe);
    
    // Wait for process to finish and return error data if any
    ph.wait();
    if (!boost::filesystem::exists(outputPath))
    {
      Poco::PipeInputStream errPipeStream(errPipe);
      errPipeStream >> compileErr;
      return false;
    }
    else 
    {
      return true;
    }
  }

  const string& grader_base::fileName() const
  {
    return m_fileName;
  }

}