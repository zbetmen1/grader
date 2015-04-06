#ifndef GRADER_LOG_HPP
#define GRADER_LOG_HPP

// STL headers
#include <stdexcept>
#include <sstream>

// Unix headers
#include <syslog.h>

namespace grader 
{
  class log;
}

template <typename T>
grader::log& operator<<(grader::log& logger, T arg);

namespace grader 
{
  class log_io_error: public std::runtime_error
  {
  public:
    explicit log_io_error(const char* arg)
    : std::runtime_error{arg}
    {}
  };
  
  class log_construction_error: public std::runtime_error
  {
  public:
    explicit log_construction_error(const char* arg)
    : std::runtime_error{arg}
    {}
  };
  
  class log 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    enum class severity: int
    {
      INVALID = -1,
      DEBUG = LOG_DEBUG,
      INFO = LOG_INFO,
      WARNING = LOG_WARNING,
      ERROR = LOG_ERR,
      FATAL = LOG_EMERG
    };
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    std::ostringstream m_buffer;
    severity m_currentSeverity;
    const severity m_severity;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit log(severity appSeverity);
    ~log();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////    
    
    static log& instance(severity appSeverity = severity::INVALID);

    void flush();
    
    static log& set_severity(severity currentSeverity);
    
  public:
    static log& debug();
    
    static log& info();
    
    static log& warning();
    
    static log& error();
    
    static log& fatal();
    
    template<typename T>
    friend log& ::operator<<(log& logger, T arg);
  };
}

//////////////////////////////////////////////////////////////////////////////
// Severity operators
//////////////////////////////////////////////////////////////////////////////
bool operator==(grader::log::severity lhs, grader::log::severity rhs);

bool operator!=(grader::log::severity lhs, grader::log::severity rhs);

bool operator<(grader::log::severity lhs, grader::log::severity rhs);

bool operator>(grader::log::severity lhs, grader::log::severity rhs);

bool operator<=(grader::log::severity lhs, grader::log::severity rhs);

bool operator>=(grader::log::severity lhs, grader::log::severity rhs);

template <typename T>
grader::log& operator<<(grader::log& logger, T arg) 
{
  logger.m_buffer << arg;
  return logger;
}

using glog = grader::log;

#endif // GRADER_LOG_HPP