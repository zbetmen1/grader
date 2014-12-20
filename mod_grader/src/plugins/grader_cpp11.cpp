#include "grader_cpp11.hpp"

using namespace std;

const vector<const char*> grader_cpp11::m_extensions{".cc", ".cpp", ".cxx"};

grader_cpp11::grader_cpp11()
: grader::grader_base()
{
}

const std::vector< const char* >& grader_cpp11::extensions() const
{
  return m_extensions;
}

const char* grader_cpp11::language() const
{
  return "cpp11";
}

const char* grader_cpp11::name() const
{
    return "grader_cpp11";
}

bool grader_cpp11::is_compilable() const
{
  return true;
}

std::string grader_cpp11::compiler() const
{
  return "g++";
}

void grader_cpp11::compiler_flags(std::string& flags) const
{
  flags += " -std=c++11 ";
}

std::string grader_cpp11::compiler_filename_flag() const
{
  return "-o";
}

bool grader_cpp11::is_compiling_from_stdin() const
{
  return false;
}

bool grader_cpp11::is_interpreted() const
{
  return false;
}
