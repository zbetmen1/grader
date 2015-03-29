// Project headers
#include "process.hpp"

// STL headers
#include <csignal>
#include <utility>

// Unix headers
#include <sys/wait.h>

using namespace std;

namespace grader 
{
  //////////////////////////////////////////////////////////////////////////////
  // Environment implementation
  //////////////////////////////////////////////////////////////////////////////
  enviroment::enviroment()
  {}

  void enviroment::put(const string& name, const string& val)
  {
    m_buff.push_back(name + '=' + val);
  }

  string enviroment::get(const string& name) const
  {
    for (const auto& envVar : m_buff)
    {
      auto equalityPos = envVar.find('=');
      string tmpName = envVar.substr(0, equalityPos);
      if (name == tmpName)
        return move(envVar.substr(equalityPos + 1));
    }
    
    return "";
  }

  const vector< string >& enviroment::data() const
  {
    return m_buff;
  }

  
  //////////////////////////////////////////////////////////////////////////////
  // Process implementation
  //////////////////////////////////////////////////////////////////////////////
  const vector<string> process::no_args{};
  
  process::process(const string& executable, const vector< string >& args, 
                  pipe_ostream* stdinStream, pipe_istream* stdoutStream, pipe_istream* stderrStream, 
                  const string& workingDir, const enviroment& e)
  {
    // Get environment variables buffer
    const vector<string>& envVars = e.data();
    
    // Get working directory
    const char* cWorkDir = workingDir.empty() ? nullptr : workingDir.c_str();
    
    // Get command line arguments
    vector<char*> argv = command_line_args(executable, args);
    
    // Create pipes
    grader::pipe stdinPipe, stdoutPipe, stderrPipe;
    
    // Fork child
    handle child = fork();
    if (child == invalid_handle)
    {
      throw process_exception(::strerror(errno));
    }
    else if (child) // Parent
    {
      // Initialize child handle
      m_childHandle = child;
      
      // Open pipe streams
      set_up_parent_pipes(stdinPipe, stdoutPipe, stderrPipe, 
                          stdinStream, stdoutStream, stderrStream);
    }
    else //Child
    {
      // Set up working directory
      if (cWorkDir)
      {
        if (::chdir(cWorkDir) == -1)
          throw process_exception(::strerror(errno));
      }
      
      // Set up environment variables
      for (const auto& envVar : envVars)
      {
        const char* cEnvVar = envVar.data();
        if (::putenv(const_cast<char*>(cEnvVar)) != 0)
          throw process_exception(::strerror(errno));
      }
      
      // Set up child pipes
      set_up_child_pipes(stdinPipe, stdoutPipe, stderrPipe, 
                          stdinStream, stdoutStream, stderrStream);
      
      // Replace process image
      ::execvp(argv[0], argv.data());
    }
  }
  
  process::process()
  : m_childHandle(invalid_handle)
  {}
  
  process::process(process&& oth)
  : m_childHandle(oth.m_childHandle)
  {
    oth.m_childHandle = invalid_handle;
  }
  
  process& process::operator=(process&& oth)
  {
    if (&oth != this)
    {
      this->wait();
      m_childHandle = oth.m_childHandle;
      oth.m_childHandle = invalid_handle;
    }
    return *this;
  }
  
  process::~process()
  {
    destroy();
  }
  
  boost::optional<int> process::finished()
  {
    int exitCode;
    handle result = ::waitpid(m_childHandle, &exitCode, WNOHANG);
    if (result == invalid_handle)
      throw process_exception(::strerror(errno));
    else if (result)
      return boost::optional<int>{exitCode};
    else 
      return boost::optional<int>{};
  }
  
  int process::wait()
  {
    int exitCode;
    handle result = ::waitpid(m_childHandle, &exitCode, 0);
    if (result == invalid_handle)
      throw process_exception(::strerror(errno));
    return exitCode;
  }
  
  void process::kill()
  {
    if (::kill(m_childHandle, SIGKILL) == -1 && errno != ESRCH)
        throw process_exception(::strerror(errno));
    this->wait();
  }
  
  vector< char* > process::command_line_args(const string& executable, const vector< string >& args) const
  {
    vector<char*> argv; argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& argument : args)
      argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr);
    return move(argv);
  }

  
  void process::set_up_parent_pipes(pipe& stdinPipe, 
                                    pipe& stdoutPipe, 
                                    pipe& stderrPipe, 
                                    pipe_ostream* stdinStream, 
                                    pipe_istream* stdoutStream, 
                                    pipe_istream* stderrStream)
  {
    try
    {
      if (stdinStream) stdinStream->open(fd_sink(stdinPipe.get_write_handle(), 
                                                 boost::iostreams::close_handle));
      if (stdoutStream) stdoutStream->open(fd_source(stdoutPipe.get_read_handle(), 
                                                     boost::iostreams::close_handle));
      if (stderrStream) stderrStream->open(fd_source(stderrPipe.get_read_handle(), 
                                                     boost::iostreams::close_handle));
    }
    catch (const exception& e)
    {
      // Clean up
      stdinPipe.close_both();
      stdoutPipe.close_both();
      stderrPipe.close_both();
      destroy();
      
      // Re-throw
      throw e;
    }
    
    // Close what needs to be closed
    stdinPipe.close_read();
    if (!stdinStream) 
      stdinPipe.close_write();
    
    stdoutPipe.close_write();
    if (!stdoutStream) 
      stdoutPipe.close_read();
    
    stderrPipe.close_write();
    if (stderrStream) 
      stderrPipe.close_read();
  }

  void process::set_up_child_pipes(pipe& stdinPipe, 
                                   pipe& stdoutPipe, 
                                   pipe& stderrPipe, 
                                   pipe_ostream* stdinStream, 
                                   pipe_istream* stdoutStream, 
                                   pipe_istream* stderrStream)
  {
    // Close what needs to be closed
    stdinPipe.close_write();
    stdoutPipe.close_read();
    stderrPipe.close_read();
    
    // Redirect pipes and close them
    try
    {
      if (stdinStream) stdinPipe.redirect_read(STDIN_FILENO);
      if (stdoutStream) stdoutPipe.redirect_write(STDOUT_FILENO);
      if (stderrStream) stderrPipe.redirect_write(STDERR_FILENO);
    }
    catch(const exception& e)
    {
      // Clean up
      stdinPipe.close_both();
      stdoutPipe.close_both();
      stderrPipe.close_both();
      
      // Re-throw
      throw e;
    }
    
    // More closing...
    stdinPipe.close_read();
    stdoutPipe.close_write();
    stderrPipe.close_write();
  }

  void process::destroy()
  {
    ::kill(m_childHandle, SIGKILL);
    int status;
    ::waitpid(m_childHandle, &status, 0);
  }
}