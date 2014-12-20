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
#include <stdexcept>
#include <iterator>
#include <sstream>

// BOOST headers
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

// Poco headers
#include <Poco/Process.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

using namespace std;
using namespace grader;

grader_base::grader_base()
{
}

void grader_base::initialize(task* t, const string& interpreterPath)
{
  m_task = t;
  m_dirPath = dir_path();
  m_executablePath = binaries_path();
  m_sourcePath = source_path();
  if (!interpreterPath.empty())
    m_interpreterPath = interpreterPath;
  boost::filesystem::create_directories(m_dirPath);
}

grader_base::~grader_base()
{
  boost::filesystem::remove_all(m_dirPath);
}

string grader_base::dir_path() const
{
  auto dpath = configuration::instance().get(configuration::BASE_DIR)->second + "/" + m_task->id();
  return move(dpath);
}

string grader_base::source_path() const
{
  return m_dirPath + "/" + m_task->file_name();
}

string grader_base::binaries_path() const
{
  return m_dirPath + "/" + strip_extension(m_task->file_name());
}

string grader_base::strip_extension(const string& fileName) const
{
  auto pointPos = fileName.find_last_of('.');
  return fileName.substr(0, pointPos);
}

string grader_base::get_extension(const string& fileName) const
{
  auto pointPos = fileName.find_last_of('.');
  return fileName.substr(pointPos + 1);
}

void grader_base::write_to_disk(const string& path, const string& content) const
{
  boost::iostreams::mapped_file_params params;
  params.path = path;
  params.new_file_size = content.length();
  params.flags = boost::iostreams::mapped_file::mapmode::readwrite;
  boost::iostreams::mapped_file mf;
  mf.open(params);
  copy(content.cbegin(), content.cend(), mf.data());
}

Poco::ProcessHandle grader_base::run_compile(string& flags, Poco::Pipe& errPipe) const
{
  vector<string> pocoFlags{configuration::instance().get(configuration::SHELL_CMD_FLAG)->second, flags};
  // Check if file needs to written to disk
  if (is_compiling_from_stdin())
  {
    // Write file content to stdin to give compiler (not writing to disk!)
    Poco::Pipe stdinPipe;
    Poco::PipeOutputStream stdinPipeStream(stdinPipe);
    stdinPipeStream << m_task->file_content();
    
    // Launch compilation
    return Poco::Process::launch(configuration::instance().get(configuration::SHELL)->second, pocoFlags, &stdinPipe, nullptr, &errPipe);
  }
  else 
  {
    // Write file to disk first and add file as a flag
    write_to_disk(m_sourcePath, m_task->file_content());
    pocoFlags[1] += " " + m_sourcePath;
    
    // Set permissions
    boost::filesystem::permissions(m_sourcePath, boost::filesystem::add_perms | boost::filesystem::others_read);
    boost::filesystem::permissions(m_dirPath, boost::filesystem::add_perms | boost::filesystem::others_write);
    
    
    // Launch compilation
    return Poco::Process::launch(configuration::instance().get(configuration::SHELL)->second, pocoFlags, nullptr, nullptr, &errPipe);
  }
}

bool grader_base::compile(string& compileErr) const
{
  // Check if we need to compile at all
  if (!is_compilable())
    return true;
  
  // Set up compiler command and it's arguments
  string flags = compiler() + " ";
  compiler_flags(flags);
  flags += compiler_filename_flag() + " " + m_executablePath;
  
  // Launch compiler
  Poco::Pipe errPipe;
  auto ph = run_compile(flags, errPipe);
  
  // Wait for process to finish and return error data if any
  int retCode = ph.wait();
  if (0 != retCode)
  {
    Poco::PipeInputStream errPipeStream(errPipe);
    compileErr = move(string(istreambuf_iterator<char>(errPipeStream),
                              istreambuf_iterator<char>()));
    return false;
  }
  return true;
}

bool grader_base::run_test(const test& t) const
{
  const subtest& in = t.first;
  const subtest& out = t.second;
  Poco::Pipe toBinaries, fromBinaries;
  
  // Case when both i/o are from standard streams
  if (subtest::subtest_i_o::STD == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_std_std(in, out, m_executablePath, toBinaries, fromBinaries);
  
  // Case when input comes as a command line args and executable output is written to stdout
  else if (subtest::subtest_i_o::CMD == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_cmd_std(in, out, m_executablePath, fromBinaries);
  
  // Case when input comes as file and executable output is written to stdout
  else if (subtest::subtest_i_o::FILE == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_file_std(in, out, m_executablePath, fromBinaries);
  
  // Case when input is stdin and output goes to file
  else if (subtest::subtest_i_o::STD == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_std_file(in, out, m_executablePath, toBinaries);
  
  // Case when input comes as cmd line arguments and output goes to file
  else if (subtest::subtest_i_o::CMD == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_cmd_file(in, out, m_executablePath);
  
  // Case when input is file and output goes to file
  else if (subtest::subtest_i_o::FILE == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_file_file(in, out, m_executablePath);
  return false;
}

bool grader_base::run_test_std_std(const subtest& in, const subtest& out, const string& executable, 
                                   Poco::Pipe& toBinaries, Poco::Pipe& fromBinaries) const
{
  auto ph = Poco::Process::launch(executable, vector<string>{}, &toBinaries, &fromBinaries, nullptr);
  Poco::PipeOutputStream toBinariesStream(toBinaries);
  Poco::PipeInputStream fromBinariesStream(fromBinaries);
  toBinariesStream << in.content().c_str();
  toBinariesStream.close();
  
  return evaluate_output_stdin(fromBinariesStream, out, ph);
}

bool grader_base::run_test_cmd_std(const subtest& in, const subtest& out, 
                                   const string& executable, Poco::Pipe& fromBinaries) const
{
  stringstream argsStream;
  argsStream << in.content().c_str();
  vector<string> args{istream_iterator<string>(argsStream), istream_iterator<string>()};
  auto ph = Poco::Process::launch(executable, args, nullptr, &fromBinaries, nullptr);
  Poco::PipeInputStream fromBinariesStream(fromBinaries);
  
  return evaluate_output_stdin(fromBinariesStream, out, ph);
}

bool grader_base::run_test_file_std(const subtest& in, const subtest& out, 
                                    const string& executable, Poco::Pipe& fromBinaries) const
{
  // Fill args and launch executable
  vector<string> args{create_file_input(in)};
  auto ph = Poco::Process::launch(executable, args, m_dirPath, nullptr, &fromBinaries, nullptr);
  Poco::PipeInputStream fromBinariesStream(fromBinaries);
  
  return evaluate_output_stdin(fromBinariesStream, out, ph);
}

bool grader_base::run_test_std_file(const subtest& in, const subtest& out, const string& executable, 
                                   Poco::Pipe& toBinaries) const
{
  string path = out.path().c_str();
  if (!boost::filesystem::path(path).is_relative()) throw runtime_error("Absolute paths are not supported!");
  auto absolutePath = m_dirPath + '/' + path;
  auto ph = Poco::Process::launch(executable, vector<string>{move(path)}, m_dirPath, &toBinaries, nullptr, nullptr);
  Poco::PipeOutputStream toBinariesStream(toBinaries);
  toBinariesStream << in.content().c_str();
  toBinariesStream.close();
  
  return evaluate_output_file(absolutePath, out, ph);
}

bool grader_base::run_test_cmd_file(const subtest& in, const subtest& out, const string& executable) const
{
  // Check path first
  string path = out.path().c_str();
  if (!boost::filesystem::path(path).is_relative()) throw runtime_error("Absolute paths are not supported!");
  auto absolutePath = m_dirPath + '/' + path;
  
  // Fill args list
  vector<string> args{move(path)};
  stringstream argsStream;
  argsStream << in.content().c_str();
  args.insert(args.begin() + 1, istream_iterator<string>(argsStream), istream_iterator<string>());
  auto ph = Poco::Process::launch(executable, args, m_dirPath, nullptr, nullptr, nullptr);
  
  return evaluate_output_file(absolutePath, out, ph);
}

bool grader_base::run_test_file_file(const subtest& in, const subtest& out, const string& executable) const
{
  vector<string> args{create_file_input(in)};
  string path = out.path().c_str();
  if (!boost::filesystem::path(path).is_relative()) throw runtime_error("Absolute paths are not supported!");
  auto absolutePath = dir_path() + '/' + path;
  args.push_back(move(path));
  auto ph = Poco::Process::launch(executable, args, dir_path(), nullptr, nullptr, nullptr);
  
  return evaluate_output_file(absolutePath, out, ph);
}

vector<string> grader_base::create_file_input(const subtest& in) const
{
  string path = in.path().c_str();
  if (!boost::filesystem::path(path).is_relative()) throw std::runtime_error("Absolute paths are forbidden.");
  string absolutePath = m_dirPath + '/' + path;
  write_to_disk(absolutePath, in.content().c_str());
  boost::filesystem::permissions(absolutePath, boost::filesystem::add_perms | boost::filesystem::others_read);
  return move(vector<string>{move(path)});
}

bool grader_base::evaluate_output_stdin(Poco::PipeInputStream& fromBinariesStream, const subtest& out, 
                                        const Poco::ProcessHandle& ph) const
{
  int retCode = ph.wait();
  if (0 != retCode) return false;
  stringstream result;
  result << fromBinariesStream.rdbuf();
  auto resStr = move(result.str());
  boost::trim(resStr);
  return resStr == out.content().c_str();
}

bool grader_base::evaluate_output_file(const string& absolutePath, const subtest& out, const Poco::ProcessHandle& ph) const
{
  int retCode = ph.wait();
  if (0 != retCode) return false;
  
  ifstream result(absolutePath);
  if (!result.is_open()) throw runtime_error("Failed to open file with result content!");
  string resStr{istreambuf_iterator<char>(result), istreambuf_iterator<char>()};
  boost::trim(resStr);
  return resStr == out.content().c_str();
}
