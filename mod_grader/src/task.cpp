/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2014  Kocic Ognjen <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Project headers
#include "task.hpp"
#include "grader.hpp"

// STL headers
#include <stdexcept>
#include <sstream>

// BOOST headers
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace boost::property_tree;

namespace grader
{
  
  task::task(const task::path_t& srcFile, const vector< task::path_t >& testInputs, const vector< task::path_t >& testOutputs,
             size_t memoryLimit, size_t timeLimit)
  : m_srcFile(srcFile), m_testInputs(testInputs), m_testOutputs(testOutputs),
  m_state(task_state::WAITING), m_id(boost::uuids::to_string(boost::uuids::random_generator()())), m_memoryLimit(memoryLimit),
  m_timeLimit(timeLimit)
  {
  }

  task::task(task::path_t&& srcFile, vector< task::path_t >&& testInputs, vector< task::path_t >&& testOutputs,
             size_t memoryLimit, size_t timeLimit)
  : m_srcFile(srcFile), m_testInputs(testInputs), m_testOutputs(testOutputs), 
  m_state(task_state::WAITING), m_id(boost::uuids::to_string(boost::uuids::random_generator()())), m_memoryLimit(memoryLimit),
  m_timeLimit(timeLimit)
  {
  }

  const task_state& task::state() const
  {
    return m_state;
  }
  
  const task::guid_t& task::id() const
  {
    return m_id;
  }
  
  const string& task::json_report() const
  {
    return m_jsonMsg;
  }

  boost::optional<string> task::is_not_valid() const
  {
    // In case when task is finished task must be valid
    if (task_state::FINISHED == state())
      return boost::optional<string>();
    
    // Check memory and time requirements
    if (m_memoryLimit < MIN_MEMORY) return boost::optional<std::string>("Invalid memory request (too low)!");
    if (m_timeLimit < MIN_TIME) return boost::optional<std::string>("Invalid time request (too low)!");
    if (m_memoryLimit > MAX_MEMORY) return boost::optional<std::string>("Invalid memory request (too high)!");
    if (m_timeLimit > MAX_TIME) return boost::optional<std::string>("Invalid time request (too high)!");
    
    // If there's no at least single test task is invalid
    if (m_testInputs.empty()) return boost::optional<string>("Did you forgot input test files?");
    if (m_testOutputs.empty()) return boost::optional<string>("Did you forgot output test files?");
    
    // Check that source path is OK
    boost::optional<string> pathOK;
    if (pathOK = check_path(m_srcFile)) return pathOK;
    
    // Check that input test files paths are OK
    for (const auto& p : m_testInputs)
      if (pathOK = check_path(p))
        return pathOK;
      
    // Check that output test files paths are OK
    for (const auto& p : m_testOutputs)
      if (pathOK = check_path(p))
        return pathOK;
    
    // Everything went OK just return empty optional
    return boost::optional<string>();
  }

  boost::optional<string> task::check_path(const task::path_t& p) const
  {
    boost::system::error_code err;
    
    // Check that path exists, that file is regular file and is not empty
    if (!exists(p, err)) return 0 != err ? err.message() : "File not found!";
    if (!is_regular_file(p, err)) return 0 != err ? err.message() : "File not regular!";
    if (!file_size(p, err)) return 0 != err ? err.message() : "File is empty (size is 0 bytes)!";
    return boost::optional<string>();
  }

  void task::write_json_error(ostringstream& errMsgStream, ostringstream& jsonStream, ptree& jsonTree)
  {
    jsonTree.put("ERROR", errMsgStream.str());
    write_json(jsonStream, jsonTree);
    m_jsonMsg = jsonStream.str();
    m_state = task_state::FINISHED;
  }
  
  void task::run_all()
  {
    // Try to get grader for source file
    auto gr = grader::create_from_src(m_srcFile, m_memoryLimit, m_timeLimit);
    ostringstream jsonStream;
    ptree jsonTree;
    
    // If there is no grader for extension of source file, construct error report and set status to finished
    if (!gr)
    {
      ostringstream errMsgStream;
      errMsgStream << "There is no grader for files with extension '" << m_srcFile.extension() << "'!";
      write_json_error(errMsgStream, jsonStream, jsonTree);
      return;
    }
    
    // Cool, we have grader and now we can compile source file
    m_state = task_state::COMPILING;
    auto compileError = gr->compile();
    if (compileError)
    {
      ostringstream errMsgStream;
      errMsgStream << "Compilation failed with error: '" << compileError.get() << "'.";
      write_json_error(errMsgStream, jsonStream, jsonTree);
      return;
    }
    
    // Student's code compiles! Let's run tests and see what we get
    m_state = task_state::RUNNING;
    jsonTree.put("SUCCESS", "");
    for (auto i = 0U, n = (unsigned)m_testInputs.size(); i < n; ++i)
    {
      auto testError = gr->run_test(m_testInputs[i], m_testOutputs[i]);
      if (testError) jsonTree.put("TEST" + to_string(i), testError.get());
      else jsonTree.put("TEST" + to_string(i), "");
    }
    
    // Create json message from property tree
    write_json(jsonStream, jsonTree);
    m_jsonMsg = jsonStream.str();
    m_state = task_state::FINISHED;
  }

  bool operator==(const task& lhs, const task& rhs)
  {
    return lhs.id() == rhs.id();
  }

  bool operator!=(const task& lhs, const task& rhs)
  {
    return !(lhs == rhs);
  }

}