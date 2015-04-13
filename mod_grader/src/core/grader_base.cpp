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
  m_dirPath = dir_path();
  m_executablePath = executable_path();
  m_sourcePath = source_path() + executable_extension();
  boost::system::error_code code;
  boost::filesystem::create_directories(m_dirPath, code);
  if (boost::system::errc::success != code)
  {
    stringstream logmsg;
    logmsg << "Error when creating directory: " << m_dirPath 
           << " Message: " << code.message()
           << " Id: " << m_task->id();
    LOG(logmsg.str(), grader::ERROR);
  }
}

grader_base::~grader_base()
{
  boost::system::error_code code;
  boost::filesystem::remove_all(m_dirPath, code);
  if (boost::system::errc::success != code)
  {
    stringstream logmsg;
    logmsg << "Error when removing directory: " << m_dirPath 
           << " Message: " << code.message()
           << " Id: " << m_task->id();
    LOG(logmsg.str(), grader::ERROR);
  }
}

string grader_base::dir_path() const
{
  const configuration& conf = configuration::instance();
  auto baseDirIt = conf.get(configuration::BASE_DIR);
  if (conf.invalid() == baseDirIt)
  {
    return "";
  }
  
  auto dpath = baseDirIt->second + "/" + m_task->id();
  return move(dpath);
}

string grader_base::source_path() const
{
  return m_dirPath + "/" + m_task->file_name();
}

string grader_base::executable_path() const
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
  if (mf.is_open())
    copy(content.cbegin(), content.cend(), mf.data());
  else 
  {
    LOG("Couldn't open memory mapped file for writing: " + path + "Id: " + m_task->id(), grader::ERROR);
  }
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
  // Check if file needs to written to disk
  if (!should_write_src_file())
  {
    Poco::Pipe stdinPipe;
    auto shell = conf.get(configuration::SHELL);
    if (conf.invalid() != shell)
    {
      auto ph = Poco::Process::launch(shell->second, pocoFlags, &stdinPipe, nullptr, &errPipe);
      Poco::PipeOutputStream stdinPipeStream(stdinPipe);
      stdinPipeStream << m_task->file_content();
      stdinPipeStream.close();
      return ph;
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
  else 
  {
    // Write file to disk first and add file as a flag
    write_to_disk(m_sourcePath, m_task->file_content());
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
}

bool grader_base::compile(string& compileErr) const
{
  // Check if we need to compile at all
  if (!is_compilable())
  {
    if (should_write_src_file())
    {
      write_to_disk(m_sourcePath, m_task->file_content());
    }
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
  write_to_disk(absolutePath, in.content().c_str());
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
  cerr << ("Result is: '" + resStr + "' Expected: '" + out.content().c_str() + "'\n");
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
