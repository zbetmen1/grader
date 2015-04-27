#ifndef LOG_HPP
#define LOG_HPP

// Project headers
#include "smart_exception.hpp"

// STL headers
#include <sstream>
#include <string>
#include <mutex>
#include <fstream>

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
  
  enum class severity: int
  { 
    debug = LOG_DEBUG,
    info = LOG_INFO,
    warn = LOG_WARNING,
    error = LOG_ERR,
    fatal = LOG_EMERG
  };
  
  struct empty_struct{};
  
  struct dummy_mutex 
  {
    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }
  };
  
  template <typename mutex_type>
  class logger 
  { 
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    severity m_log_level;
    std::stringstream m_stream;
    mutex_type m_lock;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    logger()
    : m_log_level(severity::warn)
    {
      ::openlog("grader", LOG_PID, LOG_USER);
      std::ifstream log_config("/etc/grader/log_configuration.txt");
      if (log_config)
      {
        std::string log_level;
        if (log_config >> log_level)
        {
          if (log_level == "debug")
            m_log_level = severity::debug;
          else if (log_level == "info")
            m_log_level = severity::info;
          else if (log_level == "warning")
            m_log_level = severity::warn;
          else if (log_level == "error")
            m_log_level = severity::error;
          else if (log_level == "fatal")
            m_log_level = severity::fatal;
        }
      }
    }
    
    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) = default;
    logger& operator=(logger&&) = default;
    
    ~logger()
    {
      ::closelog();
    }
    
    template <typename T>
    empty_struct append_to_stream(const T& data)
    {
      m_stream << data;
      return empty_struct{};
    }
    
  public:
    template <typename... Args>
    void log(severity level, Args... args)
    {
      if (static_cast<int>(level) > static_cast<int>(m_log_level))
        return;
      std::lock_guard<mutex_type> lock(m_lock);
      __attribute__ ((unused)) empty_struct append_all_args_hack[] = { append_to_stream(args)... };
      ::syslog(static_cast<int>(level), "%s", m_stream.str().c_str());
      m_stream.str("");
      m_stream.clear();
    }
    
    static logger& instance()
    {
      static logger l;
      return l;
    }
  };
  
  using logger_st = logger<dummy_mutex>;
  using logger_mt = logger<std::mutex>;
  
  extern logger_st& glog_st;
  extern logger_mt& glog_mt;
}

#endif // LOG_HPP