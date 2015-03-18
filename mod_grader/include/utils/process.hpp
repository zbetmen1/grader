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
  using pipe_istream = boost::iostreams::stream<boost::iostreams::file_descriptor_source>;
  using pipe_ostream = boost::iostreams::stream<boost::iostreams::file_descriptor_sink>;
  using fd_source = boost::iostreams::file_descriptor_source;
  using fd_sink = boost::iostreams::file_descriptor_sink;
  
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
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static constexpr handle invalid_handle = -1;
    static constexpr int invalid_exit_code = 256;
    static const std::vector<std::string> no_args;
    static const restrictions_array no_restrictions;
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    handle m_childHandle;
    int m_exitCode;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit process(const std::string& executable, 
                     const std::vector<std::string>& args = no_args,
                     pipe_ostream* stdinStream = nullptr,
                     pipe_istream* stdoutStream = nullptr,
                     pipe_istream* stderrStream = nullptr,
                     const restrictions_array& restrictions = no_restrictions);
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
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    void start_normal_process(const std::string& executable, 
                              const std::vector<std::string>& args,
                              pipe_ostream* stdinStream,
                              pipe_istream* stdoutStream,
                              pipe_istream* stderrStream,
                              const restrictions_array& restrictions);
    void start_timed_process(const std::string& executable, 
                             const std::vector<std::string>& args,
                             pipe_ostream* stdinStream,
                             pipe_istream* stdoutStream,
                             pipe_istream* stderrStream,
                             const restrictions_array& restrictions, 
                             restrictions_array::const_iterator timeResIt);
    void destroy();
  };
  
  class pipe 
  {
    //////////////////////////////////////////////////////////////////////////////
    // Types and constants
    //////////////////////////////////////////////////////////////////////////////
  public:
    using handle = int;
    
    static constexpr handle invalid_handle = -1;
    static constexpr int read_end = 0;
    static constexpr int write_end = 1;
  
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    handle m_read;
    handle m_write;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constructors and destructor
    //////////////////////////////////////////////////////////////////////////////
    pipe();
    ~pipe();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    handle get_read_handle() const;
    
    handle get_write_handle() const;
    
    void close_read();
    
    void close_write();   
    
    void close_both();
    
    void redirect_read(handle h) const;
    
    void redirect_write(handle h) const;
  };
}

grader::process::handle grandchild = grader::process::invalid_handle;
grader::autocall releaseGrandchild;

#endif // PROCESS_HPP