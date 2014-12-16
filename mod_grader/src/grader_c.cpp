#include "grader_c.hpp"

using namespace std;

const vector<const char*> grader_c::m_extensions{".c"};

grader_c::grader_c()
: grader::grader_base()
{
}

const vector<const char* >& grader_c::extensions() const
{
  return grader_c::m_extensions;
}

const char* grader_c::language() const
{
  return "c";
}

const char* grader_c::name() const
{
    return "grader_c";
}

bool grader_c::is_compilable() const
{
  return true;
}

string grader_c::compiler() const
{
  return "/usr/bin/gcc";
}

void grader_c::compiler_flags(vector<string >& flags) const
{
  flags = vector<string>{"-std=c99", "-O2", "-lm", "-static"};
}

string grader_c::compiler_filename_flag() const
{
  return "-o";
}

bool grader_c::is_compiling_from_stdin() const
{
  return false;
}
