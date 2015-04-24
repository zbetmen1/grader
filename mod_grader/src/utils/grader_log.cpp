// Project headers
#include "grader_log.hpp"
#include "configuration.hpp"

using namespace std;

namespace grader
{
  logger::logger()
  {
    openlog("grader", 0, 0);
  }
  
  logger::~logger()
  {
    closelog();
  }
  
  void logger::log(const std::string& msg, severity level)
  {
    syslog(level, "%s: %s", grader::to_string(level), msg.c_str());
  }

  logger& logger::instance()
  {
    static logger logger_;
    return logger_;
  }
  
  const char* to_string(severity sev)
  {
    switch (sev) {
      case severity::DEBUG:
        return "@DEBUG";
      case severity::INFO:
        return "@INFO";
      case severity::WARNING:
        return "@WARNING";
      case severity::ERROR:
        return "@ERROR";
      case severity::FATAL:
        return "@FATAL";
    }
    return "";
  }
}

void LOG(const string& msg, grader::severity level)
{
  grader::logger::instance().log(msg, level);
}
