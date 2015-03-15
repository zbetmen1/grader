#ifndef PROCESS_HPP
#define PROCESS_HPP

// Project headers
#include "autocall.hpp"

// STL headers
#include <initializer_list>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>

// Unix headers
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

// BOOST headers
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace grader 
{
  class process_exception: public std::runtime_error 
  {
  public:
    explicit process_exception(const char* arg)
    : std::runtime_error{arg}
    {}
  };
  
  class process 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    using handle = pid_t;
    
    enum limit_category 
    {
      file_size = 1,
      file_no = 2,
      cpu_time = 3,
      memory = 4
    };
    
    struct restriction 
    {
      //////////////////////////////////////////////////////////////////////////////
      // Members
      //////////////////////////////////////////////////////////////////////////////
      limit_category category;
      rlim_t limit;
      
      //////////////////////////////////////////////////////////////////////////////
      // Creators and destructor
      //////////////////////////////////////////////////////////////////////////////
      restriction(limit_category cat, rlim_t lim);
      
      //////////////////////////////////////////////////////////////////////////////
      // Operations
      //////////////////////////////////////////////////////////////////////////////
      
      void apply() const;
    };
    
    using restrictions_array = std::vector<restriction>;
    using pipe_handle = int;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static constexpr handle invalid_handle = -1;
    static constexpr int invalid_exit_code = 256;
    static constexpr int read_end = 0;
    static constexpr int write_end = 1;
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    handle m_childHandle;
    int m_exitCode;
    pipe_handle m_childStdin;
    pipe_handle m_childStdout;
    pipe_handle m_childStderr;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit process(const std::string& executable, const restrictions_array& restrictions = restrictions_array{});
    ~process();
    
    process(const process&) = delete;
    process& operator=(const process&) = delete;
    
    process(process&&);
    process& operator=(process&&);
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    
    bool finished();
    
    void wait();
    
    void kill();
    
    static void handle_grandchild_timeout(int);
    
  private:
    std::pair<autocall, autocall> open_pipe(pipe_handle (&pipes)[2]);
    static void release_pair(std::pair<autocall, autocall>& releaser);
    static void release_pair3(std::pair< grader::autocall, grader::autocall >& releaser0, 
                       std::pair< grader::autocall, grader::autocall >& releaser1, 
                       std::pair< grader::autocall, grader::autocall >& releaser2);
    
    static void fire_pair(std::pair<autocall, autocall>& releaser);
    static void fire_pair3(std::pair< grader::autocall, grader::autocall >& releaser0, 
                       std::pair< grader::autocall, grader::autocall >& releaser1, 
                       std::pair< grader::autocall, grader::autocall >& releaser2);
    
    void start_not_timed_process(const std::string& executable, const restrictions_array& restrictions);
    void start_timed_process(const std::string& executable, const restrictions_array& restrictions, 
                             restrictions_array::const_iterator timeResIt);
  };
}

grader::process::handle grandchild = grader::process::invalid_handle;
grader::autocall releaseGrandchild;

#endif // PROCESS_HPP