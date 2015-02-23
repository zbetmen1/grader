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
#include "grader_log.hpp"

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

void grader_base::initialize(task* t)
{
  m_task = t;
  m_dirPath = t->dir_path();
  m_executablePath = t->executable_path() + executable_extension();
  m_sourcePath = t->source_path();
}

grader_base::~grader_base()
{
}

boost::optional<Poco::ProcessHandle> grader_base::run_compile(string& flags, Poco::Pipe& errPipe) const
{
  // Get shell inline cmd execution flag
  const configuration& conf = configuration::instance();
  auto shellCMDFlag = conf.get(configuration::SHELL_CMD_FLAG);
  bool hasCMDFlag = true;
  if (conf.invalid() == shellCMDFlag)
  {
    LOG("Does this shell have inline command execution flag? One not found in configuration.", grader::WARNING);
  }
  
  vector<string> pocoFlags{hasCMDFlag ? shellCMDFlag->second : "", flags};
  pocoFlags[1] += " " + m_sourcePath;
  
  // Set permissions
  boost::system::error_code code;
  boost::filesystem::permissions(m_sourcePath, boost::filesystem::add_perms | boost::filesystem::others_read, code);
  if (boost::system::errc::success != code)
  {
    stringstream logmsg;
    logmsg << "Couldn't set read privileges for others on source file: " << m_sourcePath
            << " Message: " << code.message() 
            << " Id: " << m_task->id();
    LOG(logmsg.str(), grader::ERROR);
    return boost::optional<Poco::ProcessHandle>{};
  }
  boost::filesystem::permissions(m_dirPath, boost::filesystem::add_perms | boost::filesystem::others_write, code);
  if (boost::system::errc::success != code)
  {
    stringstream logmsg;
    logmsg << "Couldn't set read privileges for others on source file directory: " << m_dirPath
            << " Message: " << code.message() 
            << " Id: " << m_task->id();
    LOG(logmsg.str(), grader::ERROR);
    return boost::optional<Poco::ProcessHandle>{};
  }
  
  // Launch compilation
  auto shell = conf.get(configuration::SHELL);
  if (conf.invalid() != shell)
  {
    return Poco::Process::launch(shell->second, pocoFlags, nullptr, nullptr, &errPipe);
  }
  else 
  {
    stringstream logmsg;
    logmsg << "Shell not present in configuration, program compilation not possible! "
            << "Function: run_compile" 
            << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::ERROR);
    return boost::optional<Poco::ProcessHandle>{};
  }
}

bool grader_base::compile(string& compileErr) const
{
  // Check if we need to compile at all
  if (!is_compilable())
  {
    return true;
  }
  
  // Set up compiler command and it's arguments
  string flags; 
  flags += compiler();
  flags += ' ';
  compiler_flags(flags);
  flags += ' ';
  
  // In some cases there's no need to specify output for compiler (Java for example)
  string compilerFilenameFlag = compiler_filename_flag();
  if (!compilerFilenameFlag.empty())
  {
    flags += compilerFilenameFlag;
    flags += ' ' + m_executablePath;
  }
  
  // Launch compiler
  Poco::Pipe errPipe;
  auto phOpt = run_compile(flags, errPipe);
  if (!phOpt)
    return false;
  auto ph = *phOpt;
  
  // Wait for process to finish and return error data if any
  int retCode = ph.wait();
  if (0 != retCode)
  {
    Poco::PipeInputStream errPipeStream(errPipe);
    if (!errPipeStream.fail())
    {
      compileErr = move(string(istreambuf_iterator<char>(errPipeStream),
                                istreambuf_iterator<char>()));
    }
    else 
    {
      stringstream logmsg;
      logmsg << "Couldn't set compiler error, input stream from compiler process failed. "
             << "Function: compile"
             << "Id: " << m_task->id();
      LOG(logmsg.str(), grader::WARNING);
    }
    return false;
  }
  return true;
}

bool grader_base::run_test(const test& t) const
{
  const subtest& in = t.first;
  const subtest& out = t.second;
  Poco::Pipe toExecutable, fromExecutable;
  
  // Case when both i/o are from standard streams
  if (subtest::subtest_i_o::STD == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_std_std(in, out, m_executablePath, toExecutable, fromExecutable);
  
  // Case when input comes as a command line args and executable output is written to stdout
  else if (subtest::subtest_i_o::CMD == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_cmd_std(in, out, m_executablePath, fromExecutable);
  
  // Case when input comes as file and executable output is written to stdout
  else if (subtest::subtest_i_o::FILE == in.io() && subtest::subtest_i_o::STD == out.io())
    return run_test_file_std(in, out, m_executablePath, fromExecutable);
  
  // Case when input is stdin and output goes to file
  else if (subtest::subtest_i_o::STD == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_std_file(in, out, m_executablePath, toExecutable);
  
  // Case when input comes as cmd line arguments and output goes to file
  else if (subtest::subtest_i_o::CMD == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_cmd_file(in, out, m_executablePath);
  
  // Case when input is file and output goes to file
  else if (subtest::subtest_i_o::FILE == in.io() && subtest::subtest_i_o::FILE == out.io())
    return run_test_file_file(in, out, m_executablePath);
  
  // Unknown case
  stringstream logmsg;
  logmsg << "I/O for input or output test unknown. Couldn't run test at all, returning test failure."
         << "Function: run_test "
         << "Id: " << m_task->id();
  LOG(logmsg.str(), grader::WARNING);
  return false;
}

bool grader_base::run_test_std_std(const subtest& in, const subtest& out, const string& executable, 
                                   Poco::Pipe& toExecutable, Poco::Pipe& fromExecutable) const
{
  auto ph = start_executable_process(executable, vector<string>{}, m_dirPath, &toExecutable, &fromExecutable);
  Poco::PipeOutputStream toExecutableStream(toExecutable);
  Poco::PipeInputStream fromExecutableStream(fromExecutable);
  toExecutableStream << in.content().c_str();
  if (toExecutableStream.fail())
  {
    stringstream logmsg;
    logmsg << "Input stream to process failed. "
           << "Function: run_test_std_std "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  toExecutableStream.close();
  
  return evaluate_output_stdin(fromExecutableStream, out, ph);
}

bool grader_base::run_test_cmd_std(const subtest& in, const subtest& out, 
                                   const string& executable, Poco::Pipe& fromExecutable) const
{
  stringstream argsStream;
  argsStream << in.content().c_str();
  if (argsStream.fail())
  {
    stringstream logmsg;
    logmsg << "Writing input test content to arguments stringstream failed. "
           << "Function: run_test_cmd_std "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  vector<string> args{istream_iterator<string>(argsStream), istream_iterator<string>()};
  auto ph = start_executable_process(executable, args, m_dirPath, nullptr, &fromExecutable);
  Poco::PipeInputStream fromExecutableStream(fromExecutable);
  
  return evaluate_output_stdin(fromExecutableStream, out, ph);
}

bool grader_base::run_test_file_std(const subtest& in, const subtest& out, 
                                    const string& executable, Poco::Pipe& fromExecutable) const
{
  // Fill args and launch executable
  vector<string> args{create_file_input(in)};
  auto ph = start_executable_process(executable, args, m_dirPath, nullptr, &fromExecutable);
  Poco::PipeInputStream fromExecutableStream(fromExecutable);
  
  return evaluate_output_stdin(fromExecutableStream, out, ph);
}

bool grader_base::run_test_std_file(const subtest& in, const subtest& out, const string& executable, 
                                   Poco::Pipe& toExecutable) const
{
  using path_t = boost::filesystem::path;
  string path = out.path().c_str();
  path_t p;
  try 
  {
    p = path_t(path);
  } 
  catch (const exception& e) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path name. Error message: " << e.what()
           << " Function: run_test_std_file"
           << "Task id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  if (!p.is_relative()) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path. Absolute paths are not supported. "
           << "Function: run_test_std_file "
           << "Path: " << path << ' '
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  auto absolutePath = m_dirPath + '/' + path;
  auto ph = start_executable_process(executable, vector<string>{move(path)}, m_dirPath, &toExecutable, nullptr);
  Poco::PipeOutputStream toExecutableStream(toExecutable);
  toExecutableStream << in.content().c_str();
  if (toExecutableStream.fail())
  {
    stringstream logmsg;
    logmsg << "Input stream to process failed. "
           << "Function: run_test_std_file "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  toExecutableStream.close();
  
  return evaluate_output_file(absolutePath, out, ph);
}

bool grader_base::run_test_cmd_file(const subtest& in, const subtest& out, const string& executable) const
{
  // Check path first
  string path = out.path().c_str();
  using path_t = boost::filesystem::path;
  path_t p;
  try 
  {
    p = path_t(path);
  } 
  catch (const exception e) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path name. Error message: " << e.what()
           << " Function: run_test_cmd_file"
           << "Task id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  
  if (!p.is_relative()) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path. Absolute paths are not supported. "
           << "Function: run_test_cmd_file "
           << "Path: " << path << ' '
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  auto absolutePath = m_dirPath + '/' + path;
  
  // Fill args list
  vector<string> args{move(path)};
  stringstream argsStream;
  argsStream << in.content().c_str();
  if (argsStream.fail())
  {
    stringstream logmsg;
    logmsg << "Writing input test content to arguments stringstream failed. "
           << "Function: run_test_cmd_file "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
    return false;
  }
  args.insert(args.begin() + 1, istream_iterator<string>(argsStream), istream_iterator<string>());
  auto ph = start_executable_process(executable, args, m_dirPath, nullptr, nullptr);
  
  return evaluate_output_file(absolutePath, out, ph);
}

bool grader_base::run_test_file_file(const subtest& in, const subtest& out, const string& executable) const
{
  vector<string> args{create_file_input(in)};
  string path = out.path().c_str();
  using path_t = boost::filesystem::path;
  path_t p;
  try 
  {
    p = path_t(path);
  } 
  catch (const exception& e) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path name. Error message: " << e.what()
           << " Function: run_test_file_file"
           << "Task id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  if (!p.is_relative()) 
  {
    stringstream logmsg;
    logmsg << "Invalid output path. Absolute paths are not supported. "
           << "Function: run_test_file_file "
           << "Path: " << path << ' '
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  auto absolutePath = m_dirPath + '/' + path;
  args.push_back(move(path));
  auto ph = start_executable_process(executable, args, m_dirPath, nullptr, nullptr);
  
  return evaluate_output_file(absolutePath, out, ph);
}

vector<string> grader_base::create_file_input(const subtest& in) const
{
  string path = in.path().c_str();
  using path_t = boost::filesystem::path;
  path_t p;
  try 
  {
    p = path_t(path);
  } 
  catch (const exception& e) 
  {
    stringstream logmsg;
    logmsg << "Invalid input path name. Error message: " << e.what()
           << " Function: create_file_input"
           << "Task id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return vector<string>{};
  }
  if (!p.is_relative())
  {
    stringstream logmsg;
    logmsg << "Invalid input path. Absolute paths are not supported. "
           << "Function: create_file_input "
           << "Path: " << path << ' '
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return vector<string>{};
  }
  string absolutePath = m_dirPath + '/' + path;
  m_task->write_to_disk(absolutePath, in.content().c_str());
  boost::system::error_code code;
  boost::filesystem::permissions(absolutePath, boost::filesystem::add_perms | boost::filesystem::others_read, code);
  if (boost::system::errc::success != code)
  {
    stringstream logmsg;
    logmsg << "Couldn't set read permissions for others to read file input for program. "
           << "Function: create_file_input "
           << "Message: " << code.message()
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return vector<string>{};
  }
  return move(vector<string>{move(path)});
}

bool grader_base::evaluate_output_stdin(Poco::PipeInputStream& fromExecutableStream, const subtest& out, 
                                        const Poco::ProcessHandle& ph) const
{
  int retCode = ph.wait();
  if (0 != retCode) return false;
  if (fromExecutableStream.fail())
  {
    stringstream logmsg;
    logmsg << "Output stream from program failed. "
           << "Function: evaluate_output_stdin "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  stringstream result;
  result << fromExecutableStream.rdbuf();
  if (result.fail())
  {
    stringstream logmsg;
    logmsg << "Reading output from program to stringstream failed. "
           << "Function: evaluate_output_stdin "
           << "Id: " << m_task->id();
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  auto resStr = move(result.str());
  boost::trim(resStr);
  return resStr == out.content().c_str();
}

// TODO: Switch to memory mapped file output evaluation
bool grader_base::evaluate_output_file(const string& absolutePath, const subtest& out, const Poco::ProcessHandle& ph) const
{
  int retCode = ph.wait();
  if (0 != retCode) return false;
  
  ifstream result(absolutePath);
  if (!result.is_open())
  {
    stringstream logmsg;
    logmsg << "Failed to open file with program output. "
           << "Function: evaluate_output_file "
           << "Id: " << m_task->id()
           << "Path: " << absolutePath;
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  string resStr{istreambuf_iterator<char>(result), istreambuf_iterator<char>()};
  if (result.fail())
  {
    stringstream logmsg;
    logmsg << "Reading program output from file into string failed (stream failed). "
           << "Function: evaluate_output_file "
           << "Id: " << m_task->id()
           << "Path: " << absolutePath;
    LOG(logmsg.str(), grader::WARNING);
    return false;
  }
  boost::trim(resStr);
  return resStr == out.content().c_str();
}

Poco::ProcessHandle grader_base::start_executable_process(const string& executable, const vector< string >& args, const string& workingDir, 
                                                          Poco::Pipe* toExecutable, Poco::Pipe* fromExecutable) const
{
  return Poco::Process::launch(executable, args, workingDir, toExecutable, fromExecutable, nullptr);
}
