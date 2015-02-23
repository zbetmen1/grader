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
    using test_attributes = std::tuple<std::size_t, std::size_t, std::string>;// Memory, time and language
    enum class state : unsigned char { INVALID, WAITING, COMPILING, COMPILE_ERROR, RUNNING, FINISHED };
    static const test_attributes INVALID_TEST_ATTR;
    static const char* TESTS_FILE_NAME;
    static constexpr unsigned UUID_BYTES = 37;
    
    // Boost types 
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_uuid = char[UUID_BYTES]; // example: 2af4e3b0-ace9-4c12-9de6-674ec4b04b1f (36 chars + terminal zero)
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using mutex_type = boost::interprocess::interprocess_mutex;

  private:
    
    // Fields
    shm_string m_fileName; /**< Name of submitted file. */
    shm_uuid m_id; /**< Unique identifier for this task. This is also name of this task in shared memory. */ 
    state m_state; /**< This field is used for tracking current state of task (is task waiting in queue, or is it executing etc.).  */ 
    shm_string m_status; /**< Status is JSON encoded message to be returned when status is queried from Web module. */ 
  public:
    // Task must be created with factory function (see create_task method)
    explicit task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, const char* testsContents, 
                  std::size_t testsCLen, const boost::uuids::uuid& id);
    ~task();
    
    // Task is not copyable
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    // Task is movable so it can be kept in containers like vector
    task(task&&);
    task& operator=(task&&);
    
    // Getters
    const char* file_name() const { return m_fileName.c_str(); }
    const char* id() const { return m_id; }
    
    // API
    const char* status() const;
    state get_state() const;
    void run_all();

    // Static API
    static task* create_task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen,
                             const char* testsContent, std::size_t testsCLen);
    static bool is_valid_task_name(const char* name);
  
  private:
    static test_attributes parse_tests(const std::string& testsStr, std::vector<test>& tests);
    static void terminate_handler();
    
    // Interprocess safe status modifier
    void set_state(state newState);
    
  public:
    // Utilities
    std::string strip_extension(const std::string& fileName) const;
    std::string get_extension(const std::string& fileName) const;
    std::string dir_path() const;
    std::string source_path() const;
    std::string executable_path() const;
    void write_to_disk(const std::string& path, const std::string& content) const;
    std::string read_from_disk(const std::string& path) const;
  };
}

#endif // TASK_HPP
