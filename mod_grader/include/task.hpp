#ifndef TASK_HPP
#define TASK_HPP

// Project headers
#include "subtest.hpp"

// STL headers
#include <map>
#include <vector>
#include <string>

// BOOST headers
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

namespace grader
{
  class task
  {
  public:
    // Types and constants
    using test = std::pair<subtest, subtest>;
    
    // Boost types 
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_subtest_allocator = boost::interprocess::allocator<subtest, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_test_allocator = boost::interprocess::allocator<test, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_path = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using shm_test_vector = boost::interprocess::vector<test, shm_test_allocator>;
    using shm_uuid = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>;
    using mutex_type = boost::interprocess::interprocess_mutex;
  private:
    // Types and constants
    enum class state : unsigned char { WAITING, COMPILING, RUNNING, FINISHED };
    
    // Fields
    shm_path m_pathToSrc;
    shm_test_vector m_tests;
    shm_uuid m_id;
    state m_state;
    shm_string m_status;
    static mutex_type s_lock;
    
    // Task must be created with factory function (see create_task method)
    explicit task(const std::string& pathToSrc, const std::vector<test>& tests, const std::string& id);
  public:
    // Task is not copyable
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    // Task is movable by default (all members are movable)
    task(task&&) = default;
    task& operator=(task&&) = default;
    
    // API
    const shm_uuid& id() const;
    std::string status() const; // Must be interprocess safe
    void run_all() const;

    // Static API
    static task* create_task(const std::string& pathToSrc, const std::vector<test>& test);
  private:
    // Interprocess safe status modifier
    void set_state(state newState);
  };
}

#endif // TASK_HPP
