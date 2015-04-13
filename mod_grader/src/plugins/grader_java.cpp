#include "grader_java.hpp"

#include <algorithm>
#include <string>

using namespace std;
using namespace Poco;

const vector<const char*> grader_java::m_extensions{".java"};

grader_java::grader_java()
: grader_base()
{}

const char* grader_java::compiler() const
{
  return "javac";
}

const char* grader_java::compiler_filename_flag() const
{
  return "";
}

void grader_java::compiler_flags(std::string&) const
{}

const std::vector< const char* >& grader_java::extensions() const
{
  return m_extensions;
}

bool grader_java::is_compilable() const
{
  return true;
}

bool grader_java::is_interpreted() const
{
  return true;
}

const char* grader_java::language() const
{
  return "java";
}

const char* grader_java::name() const
{
    return "grader_java";
}

bool grader_java::should_write_src_file() const
{
  return true;
}

ProcessHandle grader_java::start_executable_process(const string& executable, 
                                                    const vector< string >& args, 
                                                    const string& workingDir, 
                                                    Pipe* toExecutable, 
                                                    Pipe* fromExecutable) const
{
  string::size_type lastSlashPos = executable.find_last_of('/');
  string realExecutable;
  if (lastSlashPos != string::npos)
  {
    realExecutable = executable.substr(lastSlashPos + 1);
  }
  vector<string> realArgs; realArgs.reserve(args.size() + 1);
  realArgs.push_back(realExecutable);
  copy(begin(args), end(args), back_inserter(realArgs));
  return grader_base::start_executable_process("java", realArgs, workingDir, toExecutable, fromExecutable);
}
