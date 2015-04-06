// Project headers
#include "log.hpp"

namespace grader 
{
  log::log(grader::log::severity appSeverity)
  : m_buffer(), m_currentSeverity(severity::INVALID), m_severity(appSeverity)
  {
    if (appSeverity == severity::INVALID)
      throw log_construction_error("Log severity specified is invalid!");
    
    ::openlog(nullptr, LOG_PID, LOG_LOCAL7);
  }

  log::~log()
  {
    flush();
    ::closelog();
  }
  
  log& log::instance(grader::log::severity appSeverity)
  {
    static log l(appSeverity);
    return l;
  }
  
  void log::flush()
  {
    if (m_currentSeverity != severity::INVALID)
    {
      ::syslog(static_cast<int>(m_currentSeverity), "%s", m_buffer.str().c_str());
      
      m_buffer.clear();
      m_currentSeverity = severity::INVALID;
    }
  }
  
  log& log::set_severity(log::severity currentSeverity)
  {
    log& l = glog::instance();
    if (l.m_currentSeverity != severity::INVALID)
      l.flush();
    
    l.m_currentSeverity = currentSeverity;
    return l;
  }
  
  log& log::debug()
  {
    return set_severity(severity::DEBUG);
  }
 
  log& log::info()
  {
    return set_severity(severity::INFO);
  }
  
  log& log::warning()
  {
    return set_severity(severity::WARNING);
  }
  
  log& log::error()
  {
    return set_severity(severity::ERROR);
  }
  
  log& log::fatal()
  {
    return set_severity(severity::FATAL);
  }
}