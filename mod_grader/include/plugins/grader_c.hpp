#ifndef GRADER_C_HPP
#define GRADER_C_HPP

// Project headers
#include "grader_base.hpp"
#include "register_creators.hpp"

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
    virtual std::string compiler() const;
    virtual void compiler_flags(std::string& flags) const;
    virtual std::string compiler_filename_flag() const;
    virtual bool is_compiling_from_stdin() const;
};

REGISTER_DYNAMIC_ST(grader_c)

#endif // GRADER_C_HPP