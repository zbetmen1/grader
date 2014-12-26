#ifndef GRADER_LOG_HPP
#define GRADER_LOG_HPP

// STL headers
#include <string>

// BOOST headers
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>

/*
 */

namespace grader 
{
  enum severity { TRACE=0, DEBUG=1, INFO=2, WARNING=3, ERROR=4, FATAL=5 };
  
  class logger
  {
    using logger_impl = boost::log::sources::severity_logger<boost::log::trivial::severity_level>;
    logger_impl m_loggerImpl;
    logger();
  public:
    static logger& instance();
    void log(const std::string& msg, severity level);
  };
}

void LOG(const std::string& msg, grader::severity level = grader::DEBUG);

#endif // GRADER_LOG_HPP