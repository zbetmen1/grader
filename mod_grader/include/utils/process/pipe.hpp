#ifndef PIPE_HPP
#define PIPE_HPP

#if defined(__linux__) || defined(__APPLE__)
#include "pipe_unix.hpp"
#endif // Unix

// STL headers
#include <stdexcept>

namespace grader 
{
  class pipe_exception: public std::runtime_error 
  {
  public:
    explicit pipe_exception(const char* arg)
    : std::runtime_error{arg}
    {}
  };
}

#endif // PIPE_HPP