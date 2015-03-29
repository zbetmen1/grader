#ifndef SAFE_PROCESS_UNIX_HPP
#define SAFE_PROCESS_UNIX_HPP

// Project headers
#include "process.hpp"

// STL headers
#include <csignal>
#include <limits>

// Unix headers
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>

namespace grader 
{
  class safe_process final: public process
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static constexpr rlim_t mem_lim_max = 100 * (1 << 20);
    static constexpr rlim_t fileno_lim_max = 10;
    static constexpr rlim_t proc_num_lim = 10;
    static constexpr rlim_t unlimited = std::numeric_limits<rlim_t>::max();
    static constexpr int invalid_exit_code = 256;
  private:
    struct dummy {};
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    int m_exitCode;
    static safe_process* current_proc_ptr;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    safe_process(dummy);
    
  public:
    explicit safe_process();
  
    explicit safe_process(const std::string& executable, const std::string& jailDir, 
                        const std::string& workDir, uid_t unprivilegedUser, 
                        rlim_t cpu, rlim_t mem, rlim_t fno,
                        const std::vector< std::string >& args = no_args, 
                        grader::pipe_ostream* stdinStream = nullptr, 
                        grader::pipe_istream* stdoutStream = nullptr, 
                        grader::pipe_istream* stderrStream = nullptr,
                        const grader::enviroment& e = enviroment());
    safe_process(const safe_process&) = delete;
    safe_process& operator=(const safe_process&) = delete;
    safe_process(safe_process&&);
    safe_process& operator=(safe_process&&);
    ~safe_process();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
  public:
    virtual boost::optional<int> finished();
    
    virtual int wait();
    
    virtual void kill();
  
    static void initialize();
  
  private:  
    static void handle_child_timeout(int);
    
    void set_limits(rlim_t mem, rlim_t fno) const;
    
    void set_timeout(rlim_t timeMs) const;
  };
}

#endif // SAFE_PROCESS_UNIX_HPP