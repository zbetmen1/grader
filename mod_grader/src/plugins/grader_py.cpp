#include "grader_py.hpp"

using namespace std;

const vector<const char*> grader_py::m_extensions{".py"};

grader_py::grader_py()
: grader::grader_base()
{
}

const vector<const char* >& grader_py::extensions() const
{
  return grader_py::m_extensions;
}

const char* grader_py::language() const
{
  return "py";
}

const char* grader_py::name() const
{
    return "grader_py";
}

bool grader_py::is_compilable() const
{
  return false;
}

string grader_py::compiler() const
{
  return "";
}

void grader_py::compiler_flags(string& ) const
{
}

string grader_py::compiler_filename_flag() const
{
  return "";
}

bool grader_py::is_compiling_from_stdin() const
{
  return false;
}

bool grader_py::is_interpreted() const
{
  return true;
}
