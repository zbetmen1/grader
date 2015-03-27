#ifndef PROCESS_UNIX_HPP
#define PROCESS_UNIX_HPP

// Project headers
#include "pipe.hpp"

// STL headers
#include <string>
#include <vector>
#include <sstream>

// BOOST headers
#include <boost/optional.hpp>

// Unix headers
#include <unistd.h>
#include <sys/types.h>

namespace grader 
{
  class enviroment 
  {
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    std::vector<std::string> m_buff;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constructors and destructor
    //////////////////////////////////////////////////////////////////////////////
    enviroment();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    void put(const std::string& name, const std::string& val);
    std::string get(const std::string& name) const;
    const std::vector<std::string>& data() const;
  };
  
  class process 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    using handle = pid_t;
    
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static constexpr handle invalid_handle = -1;
    static const std::vector<std::string> no_args;
    
  protected:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    handle m_childHandle;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit process();
    
    explicit process(const std::string& executable,
                     const std::vector<std::string>& args = no_args,
                     pipe_ostream* stdinStream = nullptr,
                     pipe_istream* stdoutStream = nullptr,
                     pipe_istream* stderrStream = nullptr,
                     const std::string& workingDir = std::string(),
                     const enviroment& e = enviroment());
    virtual ~process();
    
    process(const process&) = delete;
    process& operator=(const process&) = delete;
    
    process(process&&);
    process& operator=(process&&);
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    virtual boost::optional<int> finished();
    
    virtual int wait();
    
    virtual void kill();
  protected:
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    void destroy();
    
    std::vector<char*> command_line_args(const std::string& executable, const std::vector<std::string>& args) const;
    
    void set_up_parent_pipes(grader::pipe& stdinPipe,
                             grader::pipe& stdoutPipe,
                             grader::pipe& stderrPipe,
                             pipe_ostream* stdinStream,
                             pipe_istream* stdoutStream,
                             pipe_istream* stderrStream);
    
    void set_up_child_pipes(grader::pipe& stdinPipe,
                             grader::pipe& stdoutPipe,
                             grader::pipe& stderrPipe,
                             pipe_ostream* stdinStream,
                             pipe_istream* stdoutStream,
                             pipe_istream* stderrStream);
  };
}

#endif // PROCESS_UNIX_HPP