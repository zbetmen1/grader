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

const char* grader_c::compiler() const
{
  return "gcc";
}

void grader_c::compiler_flags(string& flags) const
{
  flags += "-g -O0 -std=c99 -lm -static -Wno-unused-result ";
}

const char* grader_c::compiler_filename_flag() const
{
  return "-o";
}

bool grader_c::should_write_src_file() const
{
  return true;
}

bool grader_c::is_interpreted() const
{
  return false;
}

