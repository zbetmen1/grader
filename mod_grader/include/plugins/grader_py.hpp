#ifndef GRADER_PY_HPP
#define GRADER_PY_HPP

// Project headers
#include "grader_base.hpp"
#include "register_creators.hpp"

class grader_py: public grader::grader_base 
{
  static const std::vector<const char*> m_extensions;
public:
    grader_py();
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
    virtual Poco::ProcessHandle start_executable_process(const std::string& executable, const std::vector< std::string >& args, 
                                                         const std::string& workingDir, Poco::Pipe* toBinaries, Poco::Pipe* fromBinaries) const;
};

REGISTER_DYNAMIC_ST(grader_py)

#endif // GRADER_PY_HPP