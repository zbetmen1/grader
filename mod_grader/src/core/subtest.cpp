// Project headers
#include "subtest.hpp"
#include "configuration.hpp"

// STL headers
#include <algorithm>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace grader 
{
  subtest::subtest(subtest::subtest_type type, std::string& cont, subtest::subtest_i_o IO, const std::string& path)
  : m_type(type), m_content(shm().get_segment_manager()), m_io(IO), m_path(shm().get_segment_manager())
  {
    // Copy content
    if (subtest_out == type)
      boost::trim(cont);
    m_content.reserve(cont.size()+1);
    m_content.insert(m_content.begin(), cont.cbegin(), cont.cend());
    
    // Copy path 
    m_path.reserve(path.size()+1);
    m_path.insert(m_path.begin(), path.cbegin(), path.cend());
  }
  
  subtest::subtest(subtest&& oth)
  : m_type(oth.m_type), m_content(boost::move(oth.m_content)), m_io(oth.m_io), m_path(boost::move(oth.m_path))
  {
  }
  
  subtest& subtest::operator=(subtest&& oth)
  {
    if (&oth != this)
    {
      m_type = oth.m_type;
      m_content = boost::move(oth.m_content);
      m_io = oth.m_io;
      m_path = boost::move(oth.m_path);
    }
    return *this;
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