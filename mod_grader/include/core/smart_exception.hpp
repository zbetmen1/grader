#include <stdexcept>

#define THROW_SMART(my_exception, msg) throw my_exception(msg, __FILE__, __LINE__)
#define DEBUG_BUILD 

namespace grader 
{
  class smart_exception: public std::runtime_error 
  {
  public:
#ifdef DEBUG_BUILD
    explicit smart_exception(const std::string& arg, const char* filename, unsigned line)
    : std::runtime_error(arg + "File name: " + filename + " Line: " + std::to_string(line))
    {}
#else
    explicit smart_exception(const std::string& arg, const char*, unsigned)
    : std::runtime_error(arg)
    {}
#endif // DEBUG_BUILD
  };
}