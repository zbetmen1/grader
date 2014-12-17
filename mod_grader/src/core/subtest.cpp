// Project headers
#include "subtest.hpp"
#include "configuration.hpp"

// STL headers
#include <algorithm>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>

using namespace std;

namespace grader 
{
  subtest::subtest(subtest::subtest_type type, const std::string& content, subtest::subtest_i_o io, const std::string& path)
  : m_type(type), m_content(shm().get_segment_manager()), m_io(io), m_path(shm().get_segment_manager())
  {
    // Copy content
    m_content.reserve(content.size());
    copy(content.cbegin(), content.cend(), m_content.begin());
    
    // Copy path 
    m_path.reserve(path.size());
    copy(path.cbegin(), path.cend(), m_path.begin());
  }
  
  subtest::subtest_i_o subtest::io_from_str(const string& ioStr)
  {
    if ("std" == ioStr)
      return subtest_i_o::STD;
    else if ("cmd" == ioStr)
      return subtest_i_o::CMD;
    else if ("file" == ioStr)
      return subtest_i_o::FILE;
    else
      throw runtime_error("Wrong input given!");
  }
}