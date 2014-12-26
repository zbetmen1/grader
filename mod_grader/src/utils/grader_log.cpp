#include "grader_log.hpp"
#include "configuration.hpp"

using namespace boost::log;
using namespace std;

namespace grader
{
  logger::logger()
  {
    const configuration& conf = configuration::instance();
    boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
    add_common_attributes();
    add_file_log(keywords::file_name = conf.get(configuration::LOG_DIR)->second + '/' + conf.get(configuration::LOG_FILE)->second, 
               keywords::rotation_size = 10 * 1024 * 1024,
               keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
               keywords::format = "<%Severity%> [%TimeStamp%]: %Message%",
               keywords::auto_flush = true
              );
    string logLevel = conf.get(configuration::LOG_LEVEL)->second;
    if ("TRACE" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::trace);
    }
    else if ("DEBUG" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::debug);
    }
    else if ("INFO" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::info);
    }
    else if ("WARNING" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::warning);
    }
    else if ("ERROR" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::error);
    }
    else if ("FATAL" == logLevel)
    {
      core::get()->set_filter(trivial::severity >= trivial::fatal);
    }
    else 
    {
      core::get()->set_filter(trivial::severity >= trivial::info);
    }
  }
  
  void logger::log(const std::string& msg, severity level)
  {
    switch (level) 
    {
      case TRACE:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::trace) << msg;
        break;
      case DEBUG:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::debug) << msg;
        break;
      case INFO:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::info) << msg;
        break;
      case WARNING:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::warning) << msg;
        break;
      case ERROR:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::error) << msg;
        break;
      case FATAL:
        BOOST_LOG_SEV(m_loggerImpl, boost::log::trivial::fatal) << msg;
        break;
    }
  }

  logger& logger::instance()
  {
    static logger logger_;
    return logger_;
  }
}

void LOG(const string& msg, grader::severity level)
{
  grader::logger::instance().log(msg, level);
}
