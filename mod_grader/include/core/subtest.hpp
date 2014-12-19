#ifndef SUBTEST_HPP
#define SUBTEST_HPP

// STL headers
#include <string>
#include <map>

// BOOST headers
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace grader
{
  class subtest
  {
  public:
    // Types and constants
    using subtest_type = bool;
    static constexpr bool subtest_in = true;
    static constexpr bool subtest_out = false;
    enum class subtest_i_o : unsigned char 
    {
      STD, CMD, FILE
    };
    
    // Boost types 
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using shm_path = shm_string;
  private:
    subtest_type m_type; // Is this test input or output test.
    shm_string m_content; // Test content (if input test then input else expected result).
    subtest_i_o m_io; // Which type of I/O will be used for this test (see html/test_example.xml for more info).
    shm_path m_path; // Path as optional parameter (again see html/test_example.xml for more info).
    
  public:
    explicit subtest(grader::subtest::subtest_type type, std::string& cont, grader::subtest::subtest_i_o IO, const std::string& path);
    
    // Subtest can be moved
    subtest(subtest&&);
    subtest& operator=(subtest&&);
    
    // API
    inline subtest_type type() const { return m_type; }
    inline const shm_string& content() const { return m_content; }
    inline subtest_i_o io() const { return m_io; }
    inline const shm_path& path() const { return m_path; }
    
    static subtest_i_o io_from_str(const std::string& ioStr);
  };
  
  using test = std::pair<subtest, subtest>;
}

#endif // SUBTEST_HPP