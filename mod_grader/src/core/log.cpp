// Project headers
#include "log.hpp"
#include "configuration.hpp"

namespace grader 
{
  const char* log::log_name = "grader_system";
  
  log::log()
  : m_severity(invalid_pr)
  {
    configuration& conf = configuration::instance();
    ::openlog(log_name, LOG_PID, get_log_facility(stoi(conf.get(configuration::log_facility))));
  }
  
  log::~log()
  {
    flush();
    ::closelog();
  }
  
  log& log::instance()
  { 
    static log l;
    return l; 
  }
  
  void log::flush() 
  {
    ::syslog(m_severity, "%s", m_stream.str().c_str());
    m_stream.clear();
    m_severity = invalid_pr;
  }
  
  int log::get_log_facility(int facilityNumber)
  {
    switch (facilityNumber) 
    {
      case 0:
        return LOG_LOCAL0;
      case 1:
        return LOG_LOCAL1;
      case 2:
        return LOG_LOCAL2;
      case 3:
        return LOG_LOCAL3;
      case 4:
        return LOG_LOCAL4;
      case 5:
        return LOG_LOCAL5;
      case 6:
        return LOG_LOCAL6;
      case 7:
        return LOG_LOCAL7;
      default:
        THROW_SMART(log_exception, "Unknown facility number!");
    }
  }
  
  log& log::operator<<(sev priority)
  {
    if (m_severity != invalid_pr)
      flush();
    m_severity = static_cast<int>(priority);
    return *this;
  }
}