#ifndef GRADER_BASE_H
#define GRADER_BASE_H

// Project headers
#include "subtest.hpp"
#include "dynamic/object.hpp"
#include "dynamic/register_creators.hpp"
#include "task.hpp"

// STL headers
#include <string>
#include <vector>

// BOOST headers
#include <boost/optional.hpp>

// Poco headers
#include <Poco/Process.h>

namespace Poco
{
  class Pipe;
  class PipeInputStream;
}

namespace grader
{ 
 
  /**
   * @brief This class is base class for every grader to be built. 
   * 
   * @details It derives from dynamic::object
   * so that user written grader classes can be created dynamically using their class names. Student's
   * programs are compiled and test cases ran by derived instances of this class.
   * 
   * @addtogroup Core
   */
  class grader_base: public dynamic::object
  {
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    
    /**< Pointer to task which is graded by this grader. */
    task* m_task;  
    
    /**< Path to task directory is cached in this member so it wouldn't be generated multiple times. */ 
    std::string m_dirPath; 
    
    /**< Path to generated (compiled) executable (also cached).  */ 
    std::string m_executablePath;
    
    /**< Path to source to be compiled (also cached).  */ 
    std::string m_sourcePath; 
  
  public:
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Empty constructor that does nothing. Grader base instances is initialized later (see grader_base::initialize).
     * 
     */
    grader_base();
    
    //@{
    /** Grader base is default copy-constructible and copy-assignable. */
    grader_base(const grader_base& oth) = default;
    grader_base& operator=(const grader_base& oth) = default;
    //@}
    
    //@{
    /** Grader base is default move-constructible and move-assignable. */
    grader_base(grader_base&& oth) = default;
    grader_base& operator=(grader_base&& oth) = default;
    // @}
    
    /**
     * @brief Destructor that does nothing, since this is base class its virtual.
     * 
     */
    virtual ~grader_base();
    
    /**
     * @brief This function initializes grader base instance with task to grade. Also sets values for cached members.
     * 
     * @param t Task to grade.
     * @return void
     */
    void initialize(task* t);
    
    //////////////////////////////////////////////////////////////////////////////
    // Specific grader informations about programming language
    //////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Returns file extensions supported by grader.
     * 
     * @return const std::vector< const char*, std::allocator< void > >&
     */    
    virtual const std::vector<const char*>& extensions() const = 0;
    
    /**
     * @brief Returns which programming language grader supports.
     * 
     * @return const char*
     */
    virtual const char* language() const = 0;
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Compiles source file.
     * 
     * @param compileErr Will be filled with compiler error message in case or compile error.
     * @return bool True if compilation succeeded, false otherwise.
     */
    virtual bool compile(std::string& compileErr) const;
    
    /**
     * @brief Executes program using test to provide input and output.
     * 
     * @param t Test to run program with.
     * @return bool True if input matches output, false otherwise.
     */
    virtual bool run_test(const test& t) const;
    
    //////////////////////////////////////////////////////////////////////////////
    // Grader informations
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief Grader class name.
     * 
     * @details Every grader can and will be constructed using its class name. So this function returns 
     * name using which new instance of that particular grader will be constructed.
     *
     * @return const char*
     */
    virtual const char* name() const { std::string gr("grader_"); return (gr + language()).c_str(); }
  
  protected:
  
    
    /**
     * @brief This function is used to tell if source should be compiled at all.
     * 
     * @details There are programming languages like C and C++ which require that source file
     * must be compiled before program execution. On the other hand script languages like Python
     * or Bash or JavaScript don't require compilation of source which is interpreted as is.
     * 
     * @return bool True if source should be compiled, false otherwise.
     */
    virtual bool is_compilable() const = 0;
    
    /**
     * @brief Path to compiler executable.
     * 
     * @return const char*
     */
    virtual const char* compiler() const = 0;
    
    /**
     * @brief Returns list of compiler flags as a string.
     * 
     * @param flags List of flags.
     * @return void
     */
    virtual void compiler_flags(std::string& flags) const = 0;
    
    /**
     * @brief Compiler flag used to specify output name for executable.
     * 
     * @details This function isn't quite necessary for whole system to work. There are to options:
     * 1) add function that returns default executable name for compiler (grader)
     * 2) force known output name using output file name flag
     * These are only two options cause name of executable must be known in order to run it. Second
     * option was chosen for this system.
     * 
     * @return const char*
     */
    virtual const char* compiler_filename_flag() const = 0;

    /**
     * @brief Returns compiler flag used to compile source from stdin.
     * 
     * @details Some compilers like gcc have flag that allows piping file content to be
     * compiled. For gcc and g++ flag is '-x'. This can be used as an optimization to avoid
     * writing source to disk since source file can already be in RAM as a part of current
     * HTTP request.
     * 
     * @return const char*
     */    
    virtual const char* compiler_stdin_flag() const { return ""; }
    
    /**
     * @brief Returns extension for executable. 
     * 
     * @details Some programming languages are compiled to intermediate byte code and
     * these byte code files can have some extensions. For example Java compiled files
     * have '.class' extension.
     * 
     * @return const char*
     */    
    virtual const char* executable_extension() const { return ""; };
    
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    
    /**
     * @brief This function starts student's program using prepared already arguments.
     * 
     * @details Since programs created from sources written in different programming languages 
     * have different ways of starting program this function represents thin abstraction over
     * that program start. For example, compiled C or C++ program can be started as './myFancyProgram',
     * but Java compiled program must be started as 'java MyFancyProgram.class'. Overriding this
     * function in graders gives implementer this flexibility. Default way is as C or C++ programs are
     * started.
     * 
     * @param executable Path to executable.
     * @param args Command line arguments.
     * @param workingDir Program working directory.
     * @param toExecutable Input stream pipe to executable.
     * @param fromExecutable Output stream pipe from executable.
     * @return Poco::ProcessHandle Handle of started process.
     */
    virtual Poco::ProcessHandle start_executable_process(const std::string& executable, 
                                                         const std::vector< std::string >& args, 
                                                         const std::string& workingDir, Poco::Pipe* toExecutable, 
                                                         Poco::Pipe* fromExecutable) const;
    
  private:
    
    /**
     * @brief Sets directory and source permissions and launches compilation. Returns process handle
     * or empty optional.
     * 
     * @param flags Flags for compiler.
     * @param errPipe Output pipe for compile errors.
     * @return boost::optional< Poco::ProcessHandle >
     */
    boost::optional<Poco::ProcessHandle> run_compile(std::string& flags, Poco::Pipe& errPipe) const;
    
    //@{
    /** These functions all just start student's program using appropriate I/O. In their names first
     *  comes type of input and then type of output.
     */
    bool run_test_std_std(const grader::subtest& in, const grader::subtest& out, const std::string& executable, 
                          Poco::Pipe& toExecutable, Poco::Pipe& fromExecutable) const;
    bool run_test_cmd_std(const subtest& in, const subtest& out, 
                                   const std::string& executable, Poco::Pipe& fromExecutable) const;
    bool run_test_file_std(const subtest& in, const subtest& out, 
                                   const std::string& executable, Poco::Pipe& fromExecutable) const;
    bool run_test_std_file(const subtest& in, const subtest& out, const std::string& executable, 
                                   Poco::Pipe& toExecutable) const;
    bool run_test_cmd_file(const subtest& in, const subtest& out, const std::string& executable) const;
    
    bool run_test_file_file(const subtest& in, const subtest& out, const std::string& executable) const;
    //@}
    
    
    /**
     * @brief This function writes program input to test defined file. 
     * 
     * @details When input to program is given as file, content of that
     * file must be flushed to disk. In this case path to file where content
     * needs to be written is also given in XML test file section regarding that
     * test case. Path to input file is first command line argument for student's
     * program and cause of that it is convenient to return rvalue reference to vector
     * of command line arguments that will be argument for grader_base::start_executable_process.
     * 
     * @param in Input for test case.
     * @return std::vector< std::string, std::allocator< void > >
     */
    std::vector<std::string> create_file_input(const subtest& in) const;
    
    // @{
    /** This pair of functions compares output from program with predefined correct output and returns true if they match.
     *  Output of program can be written to stdout or to file on predefined path, hence two functions.
     */
    bool evaluate_output_stdin(Poco::PipeInputStream& fromExecutableStream, const grader::subtest& out, 
                               const Poco::ProcessHandle& ph) const;
    
    bool evaluate_output_file(const std::string& absolutePath, const subtest& out, 
                              const Poco::ProcessHandle& ph) const;                               
    // @}
  };
}

#endif // GRADER_BASE_H
