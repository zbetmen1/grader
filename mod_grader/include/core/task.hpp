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
  /**
   * @brief Class that represents one task that should be graded.
   *
   * @details This class keeps basic informations about one source file submission. These informations
   * are source file itself, unique task identifier, current status of task execution, state of the task 
   * and task status message. Since its creation task is residing in interprocess memory waiting to be 
   * resolved. Its unique identifier is also a handle to the task that can be used to get task from interprocess
   * memory.
   */
  class task
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    
    /**< @typedef Possible task states from creation to destruction. */ 
    enum class state : unsigned char 
    { 
      INVALID, WAITING, COMPILING, COMPILE_ERROR, RUNNING, FINISHED 
      
    };
    
    /**< @typedef Test is a pair of input and expected result. */
    using test = std::pair<subtest, subtest>;  
    
    /**< @typedef Test attributes (XML element 'test') are memory in bytes, time in milliseconds and programming language name. */ 
    using test_attributes = std::tuple<std::size_t, std::size_t, std::string>; 
    
    /**< @typedef Interprocess memory allocator for ASCI characters. */ 
    using shm_char_allocator = boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>;  
    
    /**< @typedef Interprocess memory allocated string. */ 
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_char_allocator>; 
    
    /**< @typedef Interprocess mutex. */
    using mutex_type = boost::interprocess::interprocess_mutex;  
    
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    
    /**< Attributes that test cannot have. This constant is used for reporting failure in task::parse_tests().  */ 
    static const test_attributes INVALID_TEST_ATTR;
    
    /**< Every XML file with test cases is saved under this name. */ 
    static const char* TESTS_FILE_NAME;
    
    /**< Number of bytes needed for task id. For task id example see task::shm_uuid.  */ 
    static constexpr unsigned UUID_BYTES = 37;
  
    /**< @typedef Task identifier which is known as UUID or GUID (example of UUID: 2af4e3b0-ace9-4c12-9de6-674ec4b04b1f). */ 
    using shm_uuid = char[UUID_BYTES];
  private:
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    
    /**< Name of submitted file. */
    shm_string m_fileName; 
    
    /**< Unique identifier for this task. This is also name of this task in shared memory. */ 
    shm_uuid m_id; 
    
    /**< This field is used for tracking current state of task (is task waiting in queue, or is it executing etc.).  */ 
    state m_state; 
    
    /**< Status is JSON encoded message to be returned when status is queried from Web module. */
    shm_string m_status;  
  public:
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Initializes task and creates new directory and necessary files for task.
     * 
     * @details This constructor is accepting raw char pointers cause that is the most efficient 
     * way to forward arguments. For example, when getting data from apache request there's no 
     * need to create additional C++ objects and reserve additional memory one can just forward 
     * pointers to request body buffer and corresponding offsets (temporary object would be
     * created if arguments were <em> const string& </em>).
     * 
     * @param fileName Pointer to start of source file name. Under that name file source will be saved.
     * @param fnLen Source file name length.
     * @param fileContent Pointer to start of source file content.
     * @param fcLen Source file content length.
     * @param testsContents Pointer to start of XML file with tests content.
     * @param testsCLen XML tests file length.
     * @param id Unique task identifier.
     */
    explicit task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, const char* testsContents, 
                  std::size_t testsCLen, const boost::uuids::uuid& id);
    /**
     * @brief Removes task related directory and files.
     * 
     */    
    ~task();
    
    /**
     * @brief Task is not copy-constructible.
     * 
     */
    task(const task&) = delete;
    
    /**
     * @brief Task is not copy-assignable.
     * 
     */    
    task& operator=(const task&) = delete;
    
    /**
     * @brief Task is move-constructible.
     */
    task(task&&);
    
    /**
     * @brief Task is move-assignable.
     */
    task& operator=(task&&);
    
    //////////////////////////////////////////////////////////////////////////////
    // Member access functions
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Getter for task identifier.
     * 
     * @return const char*
     */
    const char* id() const { return m_id; }
    
    /**
     * @brief Getter for task status message.
     * 
     * @return const char*
     */
    const char* status() const;
    
    /**
     * @brief Task state getter. Must be interprocess safe.
     * 
     * @return grader::task::state
     */
    state get_state() const;
  
    private:
    
    /**
     * @brief Task state setter. Must be interprocess safe.
     * 
     * @param newState New task state to be set.
     * @return void
     */
    void set_state(state newState);
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Runs all tests for this task.
     * 
     * @details This function loads shared library that has grader for programming
     * language specified in XML tests file. After that grader object for this language
     * is created by name and initialized by this task (see grader_base::initialize). Function
     * then loops through all test cases and creates report from test results in task::m_status.
     * This constructor must be public or we will get compilation error in boost::interprocess.
     * Nevertheless, one should use ONLY task::create_task factory function to create new task instance! 
     * 
     * @return void
     */
    void run_all();

    //////////////////////////////////////////////////////////////////////////////
    // Static operations
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Task factory function.
     * 
     * @param fileName Pointer to start of source file name.
     * @param fnLen Source file name length.
     * @param fileContent Pointer to start of file content.
     * @param fcLen Source file content length.
     * @param testsContent Pointer to start of XML tests file content.
     * @param testsCLen XML tests content length.
     * @return grader::task* New task instance constructed in interprocess memory.
     */
    static task* create_task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen,
                             const char* testsContent, std::size_t testsCLen);
    /**
     * @brief Checks if given C string is UUID.
     * 
     * @param name Candidate for task name.
     * @return bool True if argument has UUID format, false otherwise.
     */
    static bool is_valid_task_name(const char* name);
  
  private:
    
    /**
     * @brief Parses XML tests file content. Fills given vector with tests and returns 'test'
     * XML element attributes values (see task::test_attributes).
     * 
     * @param testsStr Tests XML file content (file is read from disk to this parameter).
     * @param tests Empty vector to be filled with test cases.
     * @return grader::task::test_attributes Test attributes.
     */
    static test_attributes parse_tests(const std::string& testsStr, std::vector<test>& tests);
    
    /**
     * @brief Custom termination handler.
     * 
     * @details In C++ when there is uncaught exception or some other big error (like bad memory access)
     * std::terminate function is called. This function calls default std::terminate_handler which only 
     * calls std::abort. std::terminate_handler can be replaced with another function with same signature.
     * 
     * Main idea of this custom termination handler is to give code chance to log fatal error informations
     * like what was task id and in which plug-in exception occurred and similar. This is done by saving
     * stack state with setjmp before sensitive part of code and then jumping to that line in case of crash
     * from task::terminate_handler using longjmp. Process where crash happened should be killed after 
     * error logging cause there's no guarantee that stack unwinding will be correct.
     * @return void
     */
    static void terminate_handler();
    
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Creates string that has file name without extension (foo.txt -> foo). 
     * 
     * @param fileName Full file name.
     * @return std::string File name without extension.
     */
    std::string strip_extension(const std::string& fileName) const;
    
    /**
     * @brief Extracts file extension from file name.
     * 
     * @param fileName Full file name.
     * @return std::string File extension for file name.
     */
    std::string get_extension(const std::string& fileName) const;
    
  public:
    
    /**
     * @brief Returns path to directory where task is writing all data.
     * 
     * @return std::string
     */    
    std::string dir_path() const;
    
    /**
     * @brief Returns path to source file for this task.
     * 
     * @return std::string
     */
    std::string source_path() const;
        
    /**
     * @brief Returns path to compiled binaries for this task.
     * 
     * @return std::string
     */
    std::string executable_path() const;
    
    /**
     * @brief Writes given file content to file on given path.
     * 
     * @param path Path to file to be written.
     * @param content File content.
     * @return void
     */
    static void write_to_disk(const std::string& path, const std::string& content);
    
    /**
     * @brief Reads file from path and returns it's content as a string. Note that rvalue reference is returned.
     * 
     * @param path Path to file which will be read.
     * @return std::string
     */
    static std::string read_from_disk(const std::string& path);
  };
}

#endif // TASK_HPP
