#include "grader_py.hpp"

#include <stdexcept>

#include <boost/filesystem.hpp>

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

const char* grader_py::compiler() const
{
  return "";
}

void grader_py::compiler_flags(string& ) const
{
}

const char* grader_py::compiler_filename_flag() const
{
  return "";
}

bool grader_py::should_write_src_file() const
{
  return true;
}

bool grader_py::is_interpreted() const
{
  return true;
}

Poco::ProcessHandle grader_py::start_executable_process(const string& executable, const vector< string >& args, 
                                                        const string& workingDir, Poco::Pipe* toBinaries, Poco::Pipe* fromBinaries) const
{
  auto source = executable + ".py";
  try
  {
    boost::filesystem::permissions(source, boost::filesystem::add_perms | 
                                         boost::filesystem::owner_exe | 
                                         boost::filesystem::group_exe |
                                         boost::filesystem::others_exe);
  } catch (const std::exception& e)
  {
  }
  return grader::grader_base::start_executable_process(source, args, workingDir, toBinaries, fromBinaries);
}

