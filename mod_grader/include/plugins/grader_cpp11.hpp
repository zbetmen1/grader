#ifndef GRADER_CPP11_HPP
#define GRADER_CPP11_HPP

// Project headers
#include "grader_base.hpp"
#include "register_creators.hpp"

class grader_cpp11: public grader::grader_base 
{
  static const std::vector<const char*> m_extensions;
public:
    grader_cpp11();
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
};

REGISTER_DYNAMIC_ST(grader_cpp11)

#endif // GRADER_CPP11_HPP