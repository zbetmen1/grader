#ifndef GRADER_LOG_HPP
#define GRADER_LOG_HPP

// STL headers
#include <string>

// UNIX headers
#include <syslog.h>

namespace grader 
{
  enum severity { DEBUG=LOG_DEBUG, INFO=LOG_INFO, WARNING=LOG_WARNING, ERROR=LOG_ERR, FATAL=LOG_EMERG };
  
  const char* to_string(severity sev);
  
  class logger
  {
    logger();
    ~logger();
  public:
    static logger& instance();
    void log(const std::string& msg, severity level);
  };
}

void LOG(const std::string& msg, grader::severity level = grader::DEBUG);

#endif // GRADER_LOG_HPP