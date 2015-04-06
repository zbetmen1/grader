// Project headers
#include "safe_process.hpp"
#include "autocall.hpp"

// Unix headers
#include <sys/time.h>
#include <sys/wait.h>

using namespace std;

namespace grader 
{
  safe_process* safe_process::current_proc_ptr = nullptr;
  
  safe_process::safe_process()
  : process(), m_exitCode(invalid_exit_code)
  {}
  
  safe_process::safe_process(const string& executable, 
                             const string& jailDir,  
                             uid_t unprivilegedUser, 
                             const string& workDir, 
                             rlim_t cpu, 
                             rlim_t mem, 
                             rlim_t fno,
                             const vector< string >& args, 
                             pipe_ostream* stdinStream, 
                             pipe_istream* stdoutStream, 
                             pipe_istream* stderrStream, 
                             const enviroment& e)
  : process(), m_exitCode(invalid_exit_code)
  {
    // Get environment variables buffer
    const vector<string>& envVars = e.data();
    
    // Get jail directory
    const char* cJailDir = jailDir.empty() ? nullptr : jailDir.c_str();
    if (!cJailDir)
      throw process_exception("Jail directory must be specified!");
    
    // Get working directory
    const char* cWorkDir = workDir.empty() ? nullptr : workDir.c_str();
    
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
      autocall handleStreamFailure{[&](){
        stdinPipe.close_both();
        stdoutPipe.close_both();
        stderrPipe.close_both();
        destroy();
      }};
      
      // Open pipe streams
      set_up_parent_pipes(stdinPipe, stdoutPipe, stderrPipe, 
                          stdinStream, stdoutStream, stderrStream);
      
      // Set current process and timeout
      set_timeout(cpu);
      current_proc_ptr = this;
      handleStreamFailure.release();
    }
    else //Child
    {
      // Ignore SIGHUP that will occur due to chroot and setuid calls
      if (::signal(SIGHUP, SIG_IGN) == SIG_ERR)
        throw process_exception(::strerror(errno));
      
      // Set up jail
      if (::chdir(cJailDir) == -1)
        throw process_exception(::strerror(errno));
      if (::chroot(cJailDir) == -1)
        throw process_exception(::strerror(errno));
      if (::setuid(unprivilegedUser) == -1)
        throw process_exception(::strerror(errno));
      
      // Set up working directory
      if (cWorkDir && ::chdir(cWorkDir) == -1)
        throw process_exception(::strerror(errno));
      
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
      
      // Set process limits
      set_limits(mem, fno);
      
      // Replace process image or throw on return
      ::execvp(argv[0], argv.data());
      throw process_exception(::strerror(errno));
    }
  }
    
  safe_process::safe_process(safe_process&& oth)
  : process(move(oth)), m_exitCode(oth.m_exitCode)
  {}

  safe_process& safe_process::operator=(safe_process&& oth)
  {
    if (&oth != this)
    {
      process::operator=(move(oth));
      m_exitCode = oth.m_exitCode;
      oth.m_exitCode = invalid_exit_code;
    }
    return *this;
  }
  
  safe_process::~safe_process()
  {
  }

  boost::optional<int> safe_process::finished()
  {
    if (m_exitCode == invalid_exit_code)
    {
      boost::optional<int> isFinished = process::finished();
      if (isFinished)
        m_exitCode = isFinished.get();
      return isFinished;
    }
    return boost::optional<int>{m_exitCode};
  }
  
  int safe_process::wait()
  {
    if (m_exitCode == invalid_exit_code)
      return m_exitCode = process::wait();
    else return m_exitCode;
  }
  
  void safe_process::kill()
  {
      grader::process::kill();
  }
  
  void safe_process::set_limits(rlim_t mem, rlim_t fno) const
  {
    // Limit max memory (both mmap() and sbrk(), brk())
    struct rlimit cLimit;
    cLimit.rlim_cur = mem > mem_lim_max ? mem_lim_max : mem;
    cLimit.rlim_max = cLimit.rlim_cur;
    if (::setrlimit(RLIMIT_AS, &cLimit) == -1)
      throw process_exception(::strerror(errno));
    
    // Limit number of open file descriptors
    cLimit.rlim_cur = fno > fileno_lim_max ? fileno_lim_max : fno;
    cLimit.rlim_max = cLimit.rlim_cur;
    if (::setrlimit(RLIMIT_NOFILE, &cLimit) == -1)
      throw process_exception(::strerror(errno));
    
    // Limit number of processes
    cLimit.rlim_cur = proc_num_lim;
    cLimit.rlim_max = cLimit.rlim_cur;
    if (::setrlimit(RLIMIT_NPROC, &cLimit) == -1)
      throw process_exception(::strerror(errno));
  }
  
  void safe_process::set_timeout(rlim_t timeMs) const
  {
    struct itimerval timer;
    
    // Call timer only once
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    
    // Calculate alarm parameters from input in milliseconds
    timer.it_value.tv_sec = timeMs / 1000;
    timer.it_value.tv_usec = (timeMs % 1000) * 1000;
    
    // Set alarm
    if (::setitimer(ITIMER_REAL, &timer, nullptr) == -1)
      throw process_exception(::strerror(errno));
  }

  void safe_process::initialize()
  {
    if (::signal(SIGALRM, &handle_child_timeout) == SIG_ERR)
      throw process_exception(::strerror(errno));
  }
  
  void safe_process::handle_child_timeout(int)
  {
    // See if child has already finished
    int status;
    int result = ::waitpid(current_proc_ptr->m_childHandle, &status, WNOHANG);
    if (result == -1) // Error
    {
      throw process_exception(::strerror(errno));
    }
    else if (!result) // Child still running
    {
      if (::kill(SIGKILL, current_proc_ptr->m_childHandle) == -1)
        throw process_exception(::strerror(errno));
    }
    else // Child finished
    {
      if (::waitpid(current_proc_ptr->m_childHandle, &status, 0) == -1)
        throw process_exception(::strerror(errno));
    }
  }
}