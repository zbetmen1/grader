#include "grader_cpp.hpp"

using namespace std;

const vector<const char*> grader_cpp::m_extensions{".cc", ".cpp", ".cxx"};

grader_cpp::grader_cpp()
: grader::grader_base()
{
}

const std::vector< const char* >& grader_cpp::extensions() const
{
  return m_extensions;
}

const char* grader_cpp::language() const
{
  return "cpp";
}

const char* grader_cpp::name() const
{
    return "grader_cpp";
}

bool grader_cpp::is_compilable() const
{
  return true;
}

std::string grader_cpp::compiler() const
{
  return "g++";
}

void grader_cpp::compiler_flags(std::string& ) const
{
  return;
}

std::string grader_cpp::compiler_filename_flag() const
{
  return "-o";
}

bool grader_cpp::is_compiling_from_stdin() const
{
  return false;
}

bool grader_cpp::is_interpreted() const
{
  return false;
}
