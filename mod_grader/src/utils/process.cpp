// Project headers
#include "process.hpp"

// STL headers
#include <cstring>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <utility>

// Unix headers
#include <errno.h>
#include <csignal>
#include <sys/wait.h>

using namespace std;

namespace grader 
{ 
  process::restriction::restriction(process::limit_category cat, rlim_t lim)
  : category(cat), limit(lim)
  {}

  void process::restriction::apply() const
  {
    // Fill limit structure
    struct rlimit climit;
    climit.rlim_cur = limit;
    climit.rlim_max = limit;
    
    // Apply restriction
    switch (category) 
    {
      case file_size:
        // Set limit
        if (::setrlimit(RLIMIT_FSIZE, &climit) == -1)
          throw process_exception(::strerror(errno));
        break;
      case file_no:
        // Set limit
        if (::setrlimit(RLIMIT_NOFILE, &climit) == -1)
          throw process_exception(::strerror(errno));
        break;
      case memory:
        // Set limits but there are 2 cases:
        
        // Malloc implemented with brk() and sbrk()
        if (::setrlimit(RLIMIT_DATA, &climit) == -1)
          throw process_exception(::strerror(errno));
        
        // Malloc implemented with mmap()
        if (::setrlimit(RLIMIT_AS, &climit) == -1)
          throw process_exception(::strerror(errno));
        break;
      case cpu_time:
        // Handle this in parent of grandchild (child actually)
        break;
      default:
        throw process_exception("Unknown restriction!");
        break;
    }
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  // Process implementation
  //////////////////////////////////////////////////////////////////////////////
  
  const vector<string> process::no_args{};
  const process::restrictions_array process::no_restrictions{};
  
  process::process(const string& executable, 
                   const vector<string>& args, 
                   pipe_ostream* stdinStream,
                   pipe_istream* stdoutStream,
                   pipe_istream* stderrStream,
                   const restrictions_array&  restrictions)
  : m_childHandle(invalid_handle), m_exitCode(invalid_exit_code)
  {
    restrictions_array::const_iterator it;
    if (restrictions.end() == (it = find_if(restrictions.begin(), restrictions.end(), 
        [=](const restriction& x) { return x.category == cpu_time; })))
      start_normal_process(executable, args, stdinStream, stdoutStream, stderrStream, restrictions);
    else 
      start_timed_process(executable, args,stdinStream, stdoutStream, stderrStream, restrictions, it);
  }

  process::process(process&& oth)
  : m_childHandle(oth.m_childHandle), m_exitCode(oth.m_exitCode)
  {
    oth.m_childHandle = invalid_handle;
    oth.m_exitCode = invalid_exit_code;
  }
  
  process& process::operator=(process&& oth)
  {
    if (&oth != this)
    {
      this->wait();
      m_childHandle = oth.m_childHandle;
      m_exitCode = oth.m_exitCode;
      
      oth.m_childHandle = invalid_handle;
      oth.m_exitCode = invalid_exit_code;
    }
    return *this;
  }
  
  process::~process()
  {
    destroy();
  }
  
  bool process::finished()
  {
    handle result = ::waitpid(m_childHandle, &m_exitCode, WNOHANG) > 0;
    if (result == invalid_handle)
      throw process_exception(::strerror(errno));
    return result > 0;
  }
  
  void process::wait()
  {
    handle result = ::waitpid(m_childHandle, &m_exitCode, 0);
    if (result == invalid_handle)
      throw process_exception(::strerror(errno));
  }
  
  void process::kill()
  {
    if (::kill(m_childHandle, SIGKILL) == -1)
        throw process_exception(::strerror(errno));
    this->wait();
  }
  
  void process::start_normal_process(const string& executable, 
                                     const vector<string>& args,
                                     pipe_ostream* stdinStream,
                                     pipe_istream* stdoutStream,
                                     pipe_istream* stderrStream,
                                     const restrictions_array&  restrictions)
  {
    // Create pipes
    grader::pipe stdinPipe, stdoutPipe, stderrPipe;
    
    // Fork process
    handle pid = fork();
    if (pid == invalid_handle)
    {
      throw process_exception(::strerror(errno));
    }
    else if (pid) // Parent
    {
      // Initialize child handle
      m_childHandle = pid;
      
      // Open pipe streams
      try
      {
        if (stdinStream) stdinStream->open(fd_sink(stdinPipe.get_write_handle(), boost::iostreams::close_handle));
        if (stdoutStream) stdoutStream->open(fd_source(stdoutPipe.get_read_handle(), boost::iostreams::close_handle));
        if (stderrStream) stderrStream->open(fd_source(stderrPipe.get_read_handle(), boost::iostreams::close_handle));
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
    else //Child
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
      
      // Apply restrictions
      for (const auto& r : restrictions)
        r.apply();
      
      // Transform command line arguments into right form
      vector<char*> argv; argv.reserve(args.size() + 2);
      argv.push_back(const_cast<char*>(executable.c_str()));
      for (const auto& argument : args)
        argv.push_back(const_cast<char*>(argument.c_str()));
      argv.push_back(nullptr);
      
      // Replace process image
      ::execvp(argv[0], argv.data());
    }
  }

  void process::start_timed_process(const string& executable, 
                                    const vector<string>& args,
                                    pipe_ostream* stdinStream,
                                    pipe_istream* stdoutStream,
                                    pipe_istream* stderrStream,
                                    const restrictions_array& restrictions, 
                                    restrictions_array::const_iterator timeResIt)
  {
    // Fork process
    handle pid = fork();
    if (pid == invalid_handle)
    {
      throw process_exception(::strerror(errno));
    }
    else if (pid) // Parent
    {
      m_childHandle = pid;
    }
    else //Child
    {
      /**  
       * So things get complicated here. To limit CPU time in milliseconds ::setitimer() function
       * must be called and signal handler for SIGPROF installed in which child must be killed.
       * Question is how to know which child to kill? If we start multiple children, that need to be killed
       * when their time expires, from parent process we have no way of knowing for which child time expired.
       * Resolution of this problem is to fork() once more and have a grandchild in which we will run the
       * executable. On the other hand in child we will initialize global pid_t (local to the child) that
       * we will use in child's signal handler to kill grandchild (we will start only one grandchild for
       * each child so we will know grandchild's pid). Parent will get exit code, or which signal killed it,
       * from grandchild cause child will artificially terminate with that exit code or it will raise that signal.
       **/  
      
      // Fork process
      handle pid2 = fork();
      if (pid2 == invalid_handle)
      {
        throw process_exception(::strerror(errno));
      }
      else if (pid2) // Child
      {
        // Assign global grandchild handle and child release autocall
        ::grandchild = pid2;
        auto releaseFunc = [=]()
        {
          ::kill(pid2, SIGKILL);
          int status;
          ::waitpid(pid2, &status, 0);
        };
        ::releaseGrandchild = autocall(releaseFunc);
        
        // Install SIGPROF handler
        if (::signal(SIGALRM, handle_grandchild_timeout) == SIG_ERR)
          throw process_exception(::strerror(errno));
        
        // Apply time restriction
        struct itimerval timer;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        timer.it_value.tv_sec = timeResIt->limit / 1000;
        timer.it_value.tv_usec = (timeResIt->limit % 1000) * 1000;
        if (::setitimer(ITIMER_REAL, &timer, 0) == -1)
          throw process_exception(::strerror(errno));
        
        // Wait for signal
        ::pause();
      }
      else // Grandchild
      {
        // Apply restrictions
        for (const auto& r : restrictions)
          r.apply();
        
        // Transform command line arguments into right form
        vector<char*> argv; argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& argument : args)
          argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);
        
        // Replace process image
        ::execvp(argv[0], argv.data());
      }
    }
  }

  void process::destroy()
  {
    ::kill(m_childHandle, SIGKILL);
    int status;
    ::waitpid(m_childHandle, &status, 0);
  }
  
  void process::handle_grandchild_timeout(int)
  {
    // First release grandchild safety autocall
    ::releaseGrandchild.release();
    
    // Analyze grandchild status
    int status;
    handle result = ::waitpid(::grandchild, &status, WNOHANG);
    
    // If we got an error
    if (result == invalid_handle)
      throw process_exception(::strerror(errno));
    
    // If child is still running kill it, wait for it and raise SIGPROF
    if (result == 0)
    {
      // Kill it
      if (::kill(::grandchild, SIGKILL) == -1)
        throw process_exception(::strerror(errno));
      
      // Wait for it
      ::waitpid(::grandchild, &status, 0);
      
      // Exit with fatal SIGALRM
      ::exit(128 + SIGALRM);
    }
    else // Child exited before timeout so exit using that exit code
      ::exit(status);
  }

  
  
  //////////////////////////////////////////////////////////////////////////////
  // Pipe stream implementations
  //////////////////////////////////////////////////////////////////////////////
  pipe::pipe()
  : m_read(invalid_handle), m_write(invalid_handle)
  {
    handle fd[2];
    if (::pipe(fd) != -1)
    {
      m_read = fd[read_end];
      m_write = fd[write_end];
    }
  }
  
  pipe::~pipe()
  {}
  
  pipe::handle pipe::get_read_handle() const
  {
    return m_read;
  }

  pipe::handle pipe::get_write_handle() const
  {
    return m_write;
  }
  
  void pipe::close_read()
  {
    if (m_read != invalid_handle)
    {
      ::close(m_read);
      m_read = invalid_handle;
    }
  }
  
  void pipe::close_write()
  {
    if (m_write != invalid_handle)
    {
      ::close(m_write);
      m_write = invalid_handle;
    }
  }

  void pipe::close_both()
  {
    close_read();
    close_write();
  }
  
  void pipe::redirect_read(pipe::handle h) const
  {
    if (::dup2(m_read, h) == -1)
      throw process_exception(::strerror(errno));
  }
  
  void pipe::redirect_write(pipe::handle h) const
  {
    if (::dup2(m_write, h) == -1)
      throw process_exception(::strerror(errno));
  }
  
}