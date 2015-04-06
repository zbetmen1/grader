#ifndef DAEMON_HPP
#define DAEMON_HPP

// Project headers
#include "process/process.hpp"

// STL headers
#include <vector>
#include <functional>
#include <string>
#include <errno.h>
#include <cstring>
#include <csignal>

// Unix headers
#include <unistd.h>

namespace grader 
{
  class daemon_exception: public std::runtime_error 
  {
  public:
    explicit daemon_exception(const char* arg)
    : std::runtime_error{arg}
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
    
    ~daemon();
    
  public:
    static daemon& instance(unsigned n = 0, 
                            const std::string& workingDir = std::string(), 
                            std::function<void(int)> f = std::function<void(int)>());
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    void set_handlers(::sighandler_t termination, 
                      ::sighandler_t failure, 
                      ::sighandler_t reinitialization) const;
    
    static void handle_daemon_termination(int);
    static void handle_child_failure(int);
    static void handle_reinitialization(int);
  };
  
  void main_loop(int workerIdx);
}

#endif // DAEMON_HPP