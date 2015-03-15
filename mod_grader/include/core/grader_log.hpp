#ifndef GRADER_LOG_HPP
#define GRADER_LOG_HPP

// STL headers
#include <stdexcept>
#include <fstream>

// BOOST headers
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

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
    enum class severity: unsigned char 
    {
      DEBUG,
      INFO,
      WARNING,
      ERROR,
      FATAL
    };
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    static const char* LOG_FILE;
    
    std::ofstream m_logFile;
    boost::interprocess::file_lock m_lock;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit log();
    ~log();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////    
    
    static log& instance();
    
    static log& severity(const char* which);
  
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

template <typename T>
grader::log& operator<<(grader::log& logger, T arg) 
{
  boost::interprocess::scoped_lock<boost::interprocess::file_lock> lock(logger.m_lock);
  logger.m_logFile << arg;
  logger.m_logFile.flush();
  return logger;
}

using glog = grader::log;

#endif // GRADER_LOG_HPP