#ifndef PROCESS_HPP
#define PROCESS_HPP

#if defined(__linux__) || defined(__APPLE__)
#include "process_unix.hpp"
#endif // Unix

// STL headers
#include <stdexcept>

namespace grader 
{
  class process_exception: public std::runtime_error 
  {
  public:
    explicit process_exception(const char* arg)
    : std::runtime_error{arg}
    {}
  };
}
#endif // PROCESS_HPP