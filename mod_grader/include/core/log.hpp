#ifndef LOG_HPP
#define LOG_HPP

// Project headers
#include "smart_exception.hpp"

// STL headers
#include <sstream>
#include <string>

// Unix headers
#include <syslog.h>

namespace grader 
{
  class log_exception: public smart_exception 
  {
  public:
    explicit log_exception(const std::string& arg, const char* filename, unsigned int line)
    : smart_exception(arg, filename, line)
    {}
  };
  
  class log 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    enum class sev: int
    { 
      debug = LOG_DEBUG,
      info = LOG_INFO,
      warn = LOG_WARNING,
      error = LOG_ERR,
      fatal = LOG_EMERG
    };

  private:
    static constexpr int invalid_pr = -1;
    static const char* log_name;
    static_assert(invalid_pr != LOG_DEBUG &&
                  invalid_pr != LOG_INFO &&
                  invalid_pr != LOG_WARNING &&
                  invalid_pr != LOG_ERR &&
                  invalid_pr != LOG_EMERG,
                  "Must change invalid priority integer constant in grader::log impl.");
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    int m_severity;
    std::stringstream m_stream;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    log();
    log(const log&) = delete;
    log& operator=(const log&) = delete;
    log(log&&) = default;
    log& operator=(log&&) = default;
    ~log();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    static int get_log_facility(int facilityNumber);
  public:
    void flush();
    
    static log& instance();
    
    template <typename T>
    log& operator << (const T& t) 
    {
      m_stream << t;
      return *this;
    }
    
    log& operator << (sev priority);
  };
  
}

#endif // LOG_HPP