#ifndef TASK_HPP
#define TASK_HPP

// Project headers
#include "subtest.hpp"

// STL headers
#include <map>
#include <vector>
#include <string>
#include <tuple>

// BOOST headers
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

// Forward declaration of boost::uuids::uuid class
namespace boost
{
  namespace uuids
  {
    class uuid;
  }
}

namespace grader
{
  class task
  {
  public:
    // Types and constants
    using test = std::pair<subtest, subtest>;
    using test_attributes = std::tuple<std::size_t, std::size_t, std::string>;
    enum class state : unsigned char { WAITING, COMPILING, COMPILE_ERROR, RUNNING, FINISHED };
    
    // Boost types 
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_subtest_allocator = boost::interprocess::allocator<subtest, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_test_allocator = boost::interprocess::allocator<test, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_path = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using shm_test_vector = boost::interprocess::vector<test, shm_test_allocator>;
    using shm_uuid = char[37]; // example: 2af4e3b0-ace9-4c12-9de6-674ec4b04b1f (36 chars + terminal zero)
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using mutex_type = boost::interprocess::interprocess_mutex;

  private:
    
    // Fields
    shm_string m_fileName; /**< Name of submitted file. */
    shm_string m_fileContent; /**< Source code from submitted file. */
    shm_test_vector m_tests; /**< List of test that compiled source code should pass. */ 
    shm_uuid m_id; /**< Unique identifier for this task. This is also name of this task in shared memory. */ 
    std::size_t m_memoryBytes; /**< Maximum memory that should compiled source use when executing. */ 
    std::size_t m_timeMS; /**< Maximum allowed time for execution of compiled source. */ 
    state m_state; /**< This field is used for tracking current state of task (is task waiting in queue, or is it executing etc.).  */ 
    shm_string m_status; /**< Status is JSON encoded message to be returned when status is queried from Web module. */
    char m_language[16]; /**< Language in which source code is written. */ 
  public:
    // Task must be created with factory function (see create_task method)
    explicit task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, 
                  grader::task::shm_test_vector&& tests, const boost::uuids::uuid& id, std::size_t memoryBytes,
                  std::size_t timeMS, const std::string& language);
    
    // Task is not copyable
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    // Task is movable by default (all members are movable)
    task(task&&);
    task& operator=(task&&);
    
    // Getters
    const char* file_name() const { return m_fileName.c_str(); }
    const char* file_content() const { return m_fileContent.c_str(); }
    const char* id() const { return m_id; }
    
    // API
    const char* status() const; // Must be interprocess safe
    state get_state() const;
    void run_all();

    // Static API
    static task* create_task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen,
                             const char* testsContent, std::size_t testsCLen);
    static bool is_valid_task_name(const char* name);
  private:
    static test_attributes parse_tests(const char* testsContent, std::size_t testsCLen, grader::task::shm_test_vector& tests);
    
    // Interprocess safe status modifier
    void set_state(state newState);
  };
}

#endif // TASK_HPP
