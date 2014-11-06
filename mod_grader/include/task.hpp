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

#ifndef TASK_H
#define TASK_H

// STL headers
#include <vector>
#include <string>
#include <cstddef>
#include <type_traits>
#include <functional>

// BOOST headers
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/property_tree/ptree.hpp>

namespace grader
{
  enum class task_state: unsigned
  {
    WAITING, COMPILING, RUNNING, FINISHED
  };
  
  class task
  {
  public:
    // Common types
    using path_t = boost::filesystem::path;
    using guid_t = std::string;
    
    // Constants
    // TODO: Move this to grader and make it xml configurable?
    static constexpr std::size_t MIN_MEMORY = 1U;
    static constexpr std::size_t MAX_MEMORY = static_cast<std::size_t>(1) << 28; // 256MB is upper limit
    static constexpr std::size_t MIN_TIME = 1U;
    static constexpr std::size_t MAX_TIME = 30 * 1000; // 30s is upper limit
  private:
    // Fields
    path_t m_srcFile;  // src file for program
    std::vector<path_t> m_testInputs; // test input
    std::vector<path_t> m_testOutputs; // wanted output
    task_state m_state; // current state of task
    guid_t m_id; // unique identifier
    std::size_t m_memoryLimit; // In bytes
    std::size_t m_timeLimit; // In milliseconds
    std::string m_jsonMsg; // Final message about this task
  public:
    // Classic and cheap constructor
    explicit task(const path_t& srcFile, const std::vector<path_t>& testInputs, const std::vector<path_t>& testOutputs,
                  std::size_t memoryLimit, std::size_t timeLimit);
    explicit task(path_t&& srcFile, std::vector<path_t>&& testInputs, std::vector<path_t>&& testOutputs,
                  std::size_t memoryLimit, std::size_t timeLimit);
    
    // Task is not copyable
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    // Since every field is movable task is default movable (constructible and assignable)
    task(task&& oth) = default;
    task& operator=(task&& oth) = default;
    
    // Getting current state for task
    const task_state& state() const;
    
    // Task unique identifier
    const guid_t& id() const;
    
    // Final report when task finished (returns empty string if task state is not finished)
    const std::string& json_report() const;
    
    // Returns message explaining why this task isn't valid, or empty optional for valid task
    boost::optional<std::string> is_not_valid() const;
    
    // Runs all tests 
    void run_all();
  private:
    // Checks that path exists, that it is a regular file, finally checks that size of file is not 0 bytes
    // Returns message explaining which check failed or empty optional
    boost::optional<std::string> check_path(const path_t& p) const;
    
    // Writes error to json field
    void write_json_error(std::ostringstream& errMsgStream, std::ostringstream& jsonStream, boost::property_tree::ptree& jsonTree);
  };
  
  // Tasks can be compared on equality
  bool operator==(const task& lhs, const task& rhs);
  bool operator!=(const task& lhs, const task& rhs);
}

// Specialize std::hash so grader::task can be used with unordered containers
namespace std
{
  template<>
  struct hash<grader::task>
  {
    std::size_t operator()(const grader::task& t) const
    {
      return std::hash<grader::task::guid_t>()(t.id());
    }
  };
}

// Check that grader::task is movable
static_assert(std::is_move_constructible<grader::task>::value && std::is_move_assignable<grader::task>::value,
              "grader::task must be movable in order to work with containers!");

#endif // TASK_H
