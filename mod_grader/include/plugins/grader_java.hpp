#ifndef GRADER_JAVA_HPP
#define GRADER_JAVA_HPP

// Project headers
#include "grader_base.hpp"
#include "register_creators.hpp"

class grader_java: public grader::grader_base 
{
  static const std::vector<const char*> m_extensions;
public:
    grader_java();
    virtual const std::vector< const char* >& extensions() const;
    virtual const char* language() const;
protected:
    virtual const char* name() const;
    virtual bool is_compilable() const;
    virtual const char* compiler() const;
    virtual void compiler_flags(std::string&) const;
    virtual const char* compiler_filename_flag() const;
    virtual bool should_write_src_file() const;
    virtual bool is_interpreted() const;
    virtual Poco::ProcessHandle start_executable_process(const std::string& executable, 
                                                         const std::vector< std::string >& args, 
                                                         const std::string& workingDir, 
                                                         Poco::Pipe* toExecutable, 
                                                         Poco::Pipe* fromExecutable) const;
};

REGISTER_DYNAMIC_ST(grader_java)

#endif // GRADER_JAVA_HPP