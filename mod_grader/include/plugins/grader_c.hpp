#ifndef GRADER_C_HPP
#define GRADER_C_HPP

// Project headers
#include "grader_base.hpp"
#include "dynamic/register_creators.hpp"

class grader_c: public grader::grader_base 
{
  static const std::vector<const char*> m_extensions;
public:
    grader_c();
    virtual const std::vector< const char* >& extensions() const;
    virtual const char* language() const;
protected:
    virtual const char* name() const;
    virtual bool is_compilable() const;
    virtual const char* compiler() const;
    virtual void compiler_flags(std::string& flags) const;
    virtual const char* compiler_filename_flag() const;
    virtual bool should_write_src_file() const;
    virtual bool is_interpreted() const;
};

REGISTER_DYNAMIC_ST(grader_c)

#endif // GRADER_C_HPP