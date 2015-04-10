// Project headers
#include "daemon.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// Daemon implementation
//////////////////////////////////////////////////////////////////////////////
namespace grader 
{
  daemon::daemon(unsigned n, const string& workingDir, function<void(int)> f)
  : m_mainLoop(f)
  {
    // Set working directory
    if (!workingDir.empty())
    {
      if (::chdir(workingDir.c_str()) == -1)
        throw daemon_exception(::strerror(errno));
    }
    
    // Daemonize current process
    if (::daemon(no_dir_change, std_fds_to_dev_null) == -1)
      throw daemon_exception(::strerror(errno));
    
    // Start workers
    m_workers.reserve(n);
    for (unsigned i = 0; i < n; ++i)
      m_workers.emplace_back(std::bind(m_mainLoop, i));
    
    // Set handlers
    set_handlers(&handle_daemon_termination, &handle_child_failure, &handle_reinitialization);
  }
  
  daemon::~daemon() {}
  
  daemon& daemon::instance(unsigned n, const string& workingDir, function<void(int)> f)
  {
    static daemon d(n, workingDir, f);
    return d;
  }
  
  void daemon::set_handlers(::sighandler_t termination, 
                            ::sighandler_t failure, 
                            ::sighandler_t reinitialization) const
  {
    if (::signal(SIGTERM, termination) == SIG_ERR)
      throw daemon_exception(::strerror(errno));
    
    if (::signal(SIGCHLD, termination) == SIG_ERR)
      throw daemon_exception(::strerror(errno));
    
    if (::signal(SIGHUP, termination) == SIG_ERR)
      throw daemon_exception(::strerror(errno));
  }
  
  void daemon::handle_daemon_termination(int)
  {
    /*
     * Calling exit function will result in calling destructors of all static objects,
     * hence destructor for daemon instance will be called.
     */
    exit(EXIT_SUCCESS);
  }
  
  void daemon::handle_child_failure(int)
  {
    daemon& d = daemon::instance();
    for (size_t i = 0; i < d.m_workers.size(); ++i)
      if (d.m_workers[i].finished())
        d.m_workers[i] = move(process(std::bind(d.m_mainLoop, i)));
  }
  
  void daemon::handle_reinitialization(int)
  {
    daemon& d = daemon::instance();
    for (process& worker : d.m_workers)
      worker.send_signal(SIGHUP);
  }
  
  void main_loop(int /*workerIdx*/)
  {
    
  }
}

//////////////////////////////////////////////////////////////////////////////
// Main function
//////////////////////////////////////////////////////////////////////////////
int main(int, char**) 
{
  grader::daemon::instance(5, "/srv/chroot/trusty_amd64/home/", &grader::main_loop);
  ::pause();
}