// Project headers
#include "grader_log.hpp"

// STL headers
#include <vector>
#include <chrono>
#include <ctime>

using namespace std;

namespace grader 
{
  const char* log::LOG_FILE = "/var/log/grader/all.log";

  log::log()
  : m_logFile(LOG_FILE, fstream::app), m_lock(LOG_FILE)
  {}
  
  log::~log()
  {
    m_logFile.close();
  }

  log& log::severity(const char* which)
  {
    log& l = log::instance();
    auto time_point = std::chrono::system_clock::now();
    std::time_t ttp = std::chrono::system_clock::to_time_t(time_point);
    return l << "[" << which << "] " << ctime(&ttp) << " ";
  }

  
  log& log::debug()
  {
    return severity("DEBUG");
  }
  
  log& log::info()
  {
    return severity("INFO");
  }

  log& log::warning()
  {
    return severity("WARNING");
  }
  
  log& log::error()
  {
    return severity("ERROR");
  }
  
  log& log::fatal()
  {
    return severity("FATAL");
  }

  log& log::instance()
  {
    static log l;
    return l;
  }

}