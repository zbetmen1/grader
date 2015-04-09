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
    task* m_task;  
    std::string m_dirPath; 
    std::string m_executablePath;
    std::string m_sourcePath; 
  
  public:
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    grader_base();
    
    grader_base(const grader_base& oth) = default;
    grader_base& operator=(const grader_base& oth) = default;
    grader_base(grader_base&& oth) = default;
    grader_base& operator=(grader_base&& oth) = default;
    virtual ~grader_base();
    
    void initialize(task* t);
    
    //////////////////////////////////////////////////////////////////////////////
    // Specific grader informations about programming language
    //////////////////////////////////////////////////////////////////////////////
    virtual const std::vector<const char*>& extensions() const = 0;
    virtual const char* language() const = 0;
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    virtual bool compile(std::string& compileErr) const;
    virtual bool run_test(const test& t) const;
    
    //////////////////////////////////////////////////////////////////////////////
    // Grader informations
    //////////////////////////////////////////////////////////////////////////////
    virtual const char* name() const = 0;
  protected:
    virtual bool is_compilable() const = 0;
    virtual const char* compiler() const = 0;
    virtual void compiler_flags(std::string& flags) const = 0;
    virtual const char* compiler_filename_flag() const = 0;
    virtual const char* compiler_stdin_flag() const { return ""; }
    virtual const char* executable_extension() const { return ""; };
    
    //////////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////////
    virtual Poco::ProcessHandle start_executable_process(const std::string& executable, 
                                                         const std::vector< std::string >& args, 
                                                         const std::string& workingDir, Poco::Pipe* toExecutable, 
                                                         Poco::Pipe* fromExecutable) const;
    
  private:
    boost::optional<Poco::ProcessHandle> run_compile(std::string& flags, Poco::Pipe& errPipe) const;
    
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
    
    
    std::vector<std::string> create_file_input(const subtest& in) const;
    
    bool evaluate_output_stdin(Poco::PipeInputStream& fromExecutableStream, const grader::subtest& out, 
                               const Poco::ProcessHandle& ph) const;
    
    bool evaluate_output_file(const std::string& absolutePath, const subtest& out, 
                              const Poco::ProcessHandle& ph) const;                               
  };
}

#endif // GRADER_BASE_H
