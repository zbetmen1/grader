#ifndef DAEMON_HPP
#define DAEMON_HPP

// Project headers
#include "process/process.hpp"
#include "smart_exception.hpp"

// STL headers
#include <vector>
#include <functional>
#include <string>
#include <errno.h>
#include <cstring>
#include <csignal>

// Unix headers
#include <unistd.h>
#include <sys/types.h>

namespace grader 
{
  class daemon_exception: public smart_exception
  {
  public:
    explicit daemon_exception(const std::string& arg, const char* filename, unsigned int line)
    : smart_exception(arg, filename, line)
    {}
  };
  
  class daemon 
  {
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    std::function<void(int)> m_mainLoop;
    std::vector<process> m_workers;
    
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static constexpr int no_dir_change = 1;
    static constexpr int std_fds_to_dev_null = 0;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    daemon(unsigned n, const std::string& workingDir,  std::function<void(int)> f);
    
    // Daemon is neither copyable nor movable
    daemon(const daemon&) = delete;
    daemon& operator=(const daemon&) = delete;
    daemon(daemon&&) = delete;
    daemon& operator=(daemon&&) = delete;
    
  public:
    static daemon& instance(unsigned n = 0, 
                            const std::string& workingDir = std::string(), 
                            std::function<void(int)> f = std::function<void(int)>());
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    void set_handlers(::sighandler_t termination, 
                      ::sighandler_t failure, 
                      ::sighandler_t reinitialization) const;
    
    static void at_exit();
                      
    static void handle_daemon_termination(int);
    static void handle_child_failure(int);
    static void handle_reinitialization(int);
  };
  
  void main_loop(int workerIdx);
}

#endif // DAEMON_HPP