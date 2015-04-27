#include "logger.hpp"

namespace grader 
{
  logger_st& glog_st  = logger_st::instance();
  logger_mt& glog_mt = logger_mt::instance();
}