#ifndef GRADER_CPP_HPP
#define GRADER_CPP_HPP

// Project headers
#include "grader_base.hpp"
#include "register_creators.hpp"

class grader_cpp: public grader::grader_base 
{
  static const std::vector<const char*> m_extensions;
public:
    grader_cpp();
    virtual const std::vector< const char* >& extensions() const;
    virtual const char* language() const;
protected:
    virtual const char* name() const;
    virtual bool is_compilable() const;
    virtual std::string compiler() const;
    virtual void compiler_flags(std::string&) const;
    virtual std::string compiler_filename_flag() const;
    virtual bool is_compiling_from_stdin() const;
    virtual bool is_interpreted() const;
};

REGISTER_DYNAMIC_ST(grader_cpp)

#endif // GRADER_CPP_HPP