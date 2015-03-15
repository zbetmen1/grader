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
  
  process::process(const string& executable, const restrictions_array&  restrictions)
  : m_childHandle(invalid_handle), m_exitCode(invalid_exit_code)
  {
    restrictions_array::const_iterator it;
    if (restrictions.end() == (it = find_if(restrictions.begin(), restrictions.end(), 
        [=](const restriction& x) { return x.category == cpu_time; })))
      start_not_timed_process(executable, restrictions);
    else 
      start_timed_process(executable, restrictions, it);
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
    ::kill(m_childHandle, SIGKILL);
    int status;
    ::waitpid(m_childHandle, &status, 0);
    ::close(m_childStdin);
    ::close(m_childStdout);
    ::close(m_childStderr);
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
  
  pair<autocall,autocall> process::open_pipe(pipe_handle (&pipes)[2])
  {
    // Open stdin pipe
    if (pipe(pipes) == -1)
      throw process_exception(::strerror(errno));
    
    // Close pipe descriptors in case of exceptions
    autocall pipeReleaseRead(&::close, pipes[read_end]);
    autocall pipeReleaseWrite(&::close, pipes[write_end]);
    return move(make_pair(move(pipeReleaseRead), move(pipeReleaseWrite)));
  }
  
  void process::release_pair(pair< autocall, autocall >& releaser)
  {
    releaser.first.release();
    releaser.second.release();
  }
  
  void process::release_pair3(pair< autocall, autocall >& releaser0, 
                              pair< autocall, autocall >& releaser1, 
                              pair< autocall, autocall >& releaser2)
  {
    release_pair(releaser0);
    release_pair(releaser1);
    release_pair(releaser2);
  }

  void process::fire_pair(pair< autocall, autocall >& releaser)
  {
    releaser.first.fire();
    releaser.second.fire();
  }
  
  void process::fire_pair3(pair< autocall, autocall >& releaser0, 
                           pair< autocall, autocall >& releaser1, 
                           pair< autocall, autocall >& releaser2)
  {
    fire_pair(releaser0);
    fire_pair(releaser1);
    fire_pair(releaser2);
  }
  
  void process::start_not_timed_process(const string& executable, const restrictions_array&  restrictions)
  {
    // Open all pipes
    pipe_handle stdinPipe[2];
    pipe_handle stdoutPipe[2];
    pipe_handle stderrPipe[2];
    auto releaseStdin = open_pipe(stdinPipe);
    auto releaseStdout = open_pipe(stdoutPipe);
    auto releaseStderr = open_pipe(stderrPipe);
  
    // Fork process
    handle pid = fork();
    if (pid == invalid_handle)
    {
      throw process_exception(::strerror(errno));
    }
    else if (pid) // Parent
    {
      // Close right fds
      if (::close(stdinPipe[read_end]) == -1)
        throw process_exception(::strerror(errno));
      if (::close(stdoutPipe[write_end]) == -1)
        throw process_exception(::strerror(errno));
      if (::close(stderrPipe[write_end]) == -1)
        throw process_exception(::strerror(errno));
      
      // Prevent automatic fds closure
      release_pair3(releaseStdin, releaseStdout, releaseStderr);
      
      // Initialize child handle and pipe fds
      m_childHandle = pid;
      m_childStdin = stdinPipe[write_end];
      m_childStdout = stdoutPipe[read_end];
      m_childStderr = stderrPipe[read_end];
    }
    else //Child
    {
      // Duplicate right fds
      if (::dup2(stdinPipe[read_end], STDIN_FILENO) == -1)
        throw process_exception(::strerror(errno));
      if (::dup2(stdoutPipe[write_end], STDOUT_FILENO) == -1)
        throw process_exception(::strerror(errno));
      if (::dup2(stderrPipe[write_end], STDERR_FILENO) == -1)
        throw process_exception(::strerror(errno));
      
      // Close all fds
      fire_pair3(releaseStdin, releaseStdout, releaseStderr);

      // Apply restrictions
      for (const auto& r : restrictions)
        r.apply();
      
      // Replace process image
      const char* cexecutable = executable.c_str();
      ::execl(cexecutable, cexecutable, static_cast<char*>(0));
    }
  }

  void process::start_timed_process(const string& executable, const restrictions_array& restrictions, 
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
        
        // Replace process image
        const char* cexecutable = executable.c_str();
        ::execl(cexecutable, cexecutable, static_cast<char*>(0));
      }
    }
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
}