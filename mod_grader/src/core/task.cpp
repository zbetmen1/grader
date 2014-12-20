// Project headers
#include "task.hpp"
#include "configuration.hpp"
#include "grader_base.hpp"
#include "shared_lib.hpp"

// STL headers
#include <algorithm>
#include <utility>
#include <sstream>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/uuid/uuid.hpp>            
#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace grader;

task::mutex_type s_lock;

task::task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, 
            shm_test_vector&& tests, const boost::uuids::uuid& id, size_t memoryBytes, size_t timeMS, const string& language)
: m_fileName(shm().get_segment_manager()), m_fileContent(shm().get_segment_manager()), m_tests(boost::move(tests)), 
m_memoryBytes(memoryBytes), m_timeMS(timeMS), m_state(state::WAITING), m_status(shm().get_segment_manager())
{
  // Copy file name
  m_fileName.reserve(fnLen + 1);
  m_fileName.insert(m_fileName.begin(), fileName, fileName + fnLen);
  
  // Copy file content
  m_fileContent.reserve(fcLen + 1);
  m_fileContent.insert(m_fileContent.begin(), fileContent, fileContent + fcLen);
  
  // Copy uuid
  const string tmp = boost::lexical_cast<std::string>(id);
  copy(tmp.cbegin(), tmp.cend(), m_id);
  m_id[tmp.size()] = '\0';
  
  // Copy language
  copy(language.cbegin(), language.cend(), m_language);
  m_language[language.size()] = '\0';
}

task::task(task&& oth)
: m_fileName(boost::move(oth.m_fileName)), m_fileContent(boost::move(oth.m_fileContent)), m_tests(boost::move(oth.m_tests)),
m_memoryBytes(oth.m_memoryBytes), m_timeMS(oth.m_timeMS), m_state(oth.m_state), m_status(boost::move(oth.m_status))
{
}

task& task::operator=(task&& oth)
{
  if (&oth != this)
  {
    m_fileName = boost::move(oth.m_fileName);
    m_fileContent = boost::move(oth.m_fileContent);
    m_tests = boost::move(oth.m_tests);
    m_memoryBytes = oth.m_memoryBytes;
    m_timeMS = oth.m_timeMS;
    m_state = oth.m_state;
    m_status = boost::move(oth.m_status);
  }
  return *this;
}

void task::run_all()
{
  // Create grader object
  const configuration& conf = configuration::instance();
  auto graderInfo = conf.get_grader(m_language);
  string libPath = conf.get(configuration::LIB_DIR)->second + "/" + 
                            configuration::get_lib_name(graderInfo);
  dynamic::shared_lib lib(libPath);
  auto graderObj = lib.make_object<grader_base>(configuration::get_grader_name(graderInfo));
  if (!graderObj)
  {
    string msg("Couldn't create grader object! Grader name: " + configuration::get_grader_name(graderInfo) 
               + " Library path: " + libPath + ".");
    throw runtime_error(msg.c_str());
  }
  graderObj->initialize(this, configuration::get_interpreter_path(graderInfo));
  
  // Compile source
  set_state(task::state::COMPILING);
  string compilationErr;
  ostringstream formater;
  if (!graderObj->compile(compilationErr))
  {
    transform(compilationErr.begin(), compilationErr.end(), compilationErr.begin(), 
              [=](char x) { return ('\n'== x) ? ' ' : x;});
    formater << "{\n\t\"STATE\" : \"COMPILE_ERROR\",\n\t\"MESSAGE\" : \"" << compilationErr << "\"\n}";
    auto jsonStr = move(formater.str());
    m_status.insert(m_status.begin(), jsonStr.cbegin(), jsonStr.cend());
    set_state(task::state::COMPILE_ERROR);
    return;
  }
  
  // Run tests
  set_state(task::state::RUNNING);
  vector<bool> testResults;
  testResults.reserve(m_tests.size());
  for (const auto& t : m_tests)
  {
    bool res = graderObj->run_test(t);
    testResults.push_back(res);
  }
  
  // Construct status message
  auto testResSize = testResults.size();
  formater << "{ \n\t\"STATE\" : \"FINISHED\",\n";
  for (decltype(testResSize) i = 0; i < testResSize - 1; ++i)
  {
    formater << "\t\"TEST" << i << "\" : " << to_string(testResults[i]) << " ,\n";
  }
  formater << "\t\"TEST" << (testResSize - 1) << "\" : " << to_string(testResults[testResSize - 1]) << " \n}";
  auto jsonStr = move(formater.str());
  m_status.insert(m_status.begin(), jsonStr.cbegin(), jsonStr.cend());
  set_state(task::state::FINISHED);
}

std::string task::status() const
{
  boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
  std::string retval;
  switch(m_state)
  {
    case state::WAITING:
      retval = "{ \"STATE\" : \"WAITING\" }";
      return move(retval);
    case state::COMPILING:
      retval = "{ \"STATE\" : \"COMPILING\" }";
      return move(retval);
    case state::RUNNING:
      retval = "{ \"STATE\" : \"RUNNING\" }";
      return move(retval);
    case state::FINISHED:
    case state::COMPILE_ERROR:
      retval.reserve(m_status.size()); // yet written to m_status, we need to lock this section
      copy(m_status.cbegin(), m_status.cend(), retval.begin());
      return move(retval);
  }
  return move(retval);
}

task* task::create_task(const char* fileName, std::size_t fnLen, const char* fileContent, 
                        std::size_t fcLen, const char* testsContent, std::size_t testsCLen)
{
  // Generate uuid
  auto uuid = boost::uuids::random_generator()();
  shm_test_vector tests(shm().get_segment_manager());
  auto memTimeReq = parse_tests(testsContent, testsCLen, tests);
  return shm().construct<task>(boost::uuids::to_string(uuid).c_str())(fileName, fnLen, fileContent, 
                                                                      fcLen, move(tests), uuid, 
                                                                      get<0>(memTimeReq), get<1>(memTimeReq),
                                                                      get<2>(memTimeReq)
                                                                      );
}

task::test_attributes task::parse_tests(const char* testsContent, std::size_t testsCLen, task::shm_test_vector& tests)
{
  // Read xml into property tree
  using namespace boost::property_tree;
  ptree pt;
  istringstream testsInput(string(testsContent, testsContent + testsCLen));
  xml_parser::read_xml(testsInput, pt, xml_parser::no_comments);
  
  // Get memory and time requirements from xml root
  auto root = pt.get_child("test");
  size_t memoryBytes = root.get<size_t>("<xmlattr>.memory");
  size_t timeMilliseconds = root.get<size_t>("<xmlattr>.time");
  string language = root.get<string>("<xmlattr>.language");
  
  // Traverse through property tree 
  auto treeItBegin = root.begin();
  auto treeItEnd = root.end();
  while (treeItBegin != treeItEnd)
  {
    // Skip attributes of test
    if ("<xmlattr>" == treeItBegin->first)
    {
      ++treeItBegin;
      continue;
    }
      
    // Handle input
    if ("input" != treeItBegin->first)
    {
      throw runtime_error("Invalid xml format! Expected 'input'!");
    }
    else 
    {
      // Get input test
      subtest in = subtest(subtest::subtest_in, treeItBegin->second.data(), 
                        subtest::io_from_str(treeItBegin->second.get<string>("<xmlattr>.type")),
                        treeItBegin->second.get("<xmlattr>.path", ""));
      
      // Advance to next xml element
      ++treeItBegin;
      if (treeItBegin == treeItEnd)
        throw runtime_error("Invalid xml format! Expected element after 'input'!");
      if ("output" != treeItBegin->first)
        throw runtime_error("Invalid xml format! Expected 'output'!");
      
      // Get output test, add new element to tests vector and advance in tree
      subtest out = subtest(subtest::subtest_out, treeItBegin->second.data(),
                      subtest::io_from_str(treeItBegin->second.get<string>("<xmlattr>.type")),
                      treeItBegin->second.get("<xmlattr>.path", ""));
      tests.emplace_back(move(in), move(out));
      const test& tmp = tests.front();
    }
    ++treeItBegin;
  }
  
  return move(make_tuple(memoryBytes, timeMilliseconds, language));
}

void task::set_state(task::state newState)
{
  boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
  m_state = newState;
}