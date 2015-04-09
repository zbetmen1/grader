#ifndef TASK_HPP
#define TASK_HPP

// Project headers
#include "subtest.hpp"

// STL headers
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <stdexcept>
#include <csignal>

// BOOST headers
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/filesystem.hpp>

// Unix headers
#include <aio.h>

namespace fs = boost::filesystem;

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
  class task_exception: public std::runtime_error 
  {
  public:
    explicit plugin_exception(const std::string& arg)
    : std::runtime_error(arg)
    {}
  };
  
  class task
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    enum class state : unsigned char 
    { 
      INVALID, WAITING, COMPILING, COMPILE_ERROR, RUNNING, FINISHED 
      
    };
    
    using test = std::pair<subtest, subtest>;  
    
    using test_attributes = std::tuple<std::size_t, std::size_t, std::string>; 
    
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;  
    
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>; 
    
    using shm_condition = boost::interprocess::interprocess_condition;
    
    using mutex_type = boost::interprocess::interprocess_mutex;  
    
    using async_io_req = struct aiocb;
    
    using byte = unsigned char;
    
    using sig_event = struct sigevent;
    
    using sig_val = union sigval;
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    static const test_attributes invalid_test_attr;
    static const std::string test_file_name;
    static const std::string source_file_name_base;
    static constexpr unsigned uuid_len = 37;
  
    using shm_uuid = char[uuid_len];
  private:
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    shm_uuid m_id;
    state m_state;
    shm_string m_status;
    async_io_req m_req[2];
    sig_event m_event;
    shm_condition m_ready;
    mutable mutex_type m_lock;
  public:
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, const char* testsContents, 
                  std::size_t testsCLen, const boost::uuids::uuid& id);
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&&);
    task& operator=(task&&);
    ~task();
    
    //////////////////////////////////////////////////////////////////////////////
    // Member access functions
    //////////////////////////////////////////////////////////////////////////////
    const char* id() const { return m_id; }
    
    void status(std::string& fillStatus) const;
    
    state get_state() const;
  private:
    void set_state(state newState);
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    fs::path dir_path() const;
    
    void run_all(int workerIdx);

    void wait_for_disk_flush_to_complete();
    
    static task* create_task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen,
                             const char* testsContent, std::size_t testsCLen);
    
    static bool is_valid_task_name(const char* name);
  private:
    static test_attributes parse_tests(const std::string& testsStr, std::vector<test>& tests);
    
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    void prepare_async_io_req(async_io_req& req, int filedes, void* buff, std::size_t nbytes, int opcode);
    
    void prepare_sig_event();
    
    static void handle_disk_flush_done(sig_val sv);
  };
}

#endif // TASK_HPP
