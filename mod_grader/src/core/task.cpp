// Project headers
#include "task.hpp"
#include "configuration.hpp"
#include "grader_base.hpp"
#include "shared_lib.hpp"

// STL headers
#include <algorithm>
#include <utility>
#include <sstream>
#include <string>
#include <functional>
#include <csetjmp>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/uuid/uuid.hpp>            
#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/filesystem.hpp>

using namespace std;
using namespace grader;

task::mutex_type s_lock;
const task::test_attributes task::INVALID_TEST_ATTR{0, 0, ""};

jmp_buf g_saveStateBeforeTerminate;

task::task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, 
            shm_test_vector&& tests, const boost::uuids::uuid& id, size_t memoryBytes, size_t timeMS, const string& language)
: m_fileName(shm().get_segment_manager()), m_fileContent(shm().get_segment_manager()), m_tests(boost::move(tests)), 
m_memoryBytes(memoryBytes), m_timeMS(timeMS), m_state(state::WAITING), m_status(shm().get_segment_manager())
{
  // Correctly handle case when client sent relative file path (extract file name)
  using path_t = boost::filesystem::path;
  path_t tmpPath;
  try 
  {
    tmpPath = path_t(fileName, fileName + fnLen);
  } 
  catch (const exception& e) 
  {
    stringstream logmsg;
    logmsg << "Invalid file name! Task id: " << id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  if (tmpPath.has_filename())
  {
    m_fileName = tmpPath.filename().c_str();
  }
  else 
  {
    stringstream logmsg;
    logmsg << "Bad file name in task constructor: " << tmpPath.c_str() << " task id: " << id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  
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
  // Fetch grader informations for programming language
  const configuration& conf = configuration::instance();
  auto graderInfo = conf.get_grader(m_language);
  
  // Check if grader for this language doesn't exists in config.xml
  if (configuration::INVALID_GR_INFO == graderInfo)
  {
    stringstream logmsg;
    logmsg << "Couldn't find grader for language: " << m_language << " in config.xml.";
    logmsg << "Task id: " << m_id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  
  // Get base path where all grader libraries are (check that lib dir is set in config.xml)
  auto baseLibPathIt = conf.get(configuration::LIB_DIR);
  if (conf.invalid() == baseLibPathIt)
  {
    stringstream logmsg;
    logmsg << "Couldn't find entry for directory where all grader libraries are in config.xml.";
    logmsg << "Task id: " << m_id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  
  // Construct library path
  string libPath = baseLibPathIt->second + "/" + 
                            configuration::get_lib_name(graderInfo);
  
  // This function is place where third party grader plugins can crash
  // whole application, so std::terminate_handler will be replaced, saved
  // and restored upon successful completition or upon failure
  std::terminate_handler defaultHandler = set_terminate(&task::terminate_handler);
  if (setjmp(g_saveStateBeforeTerminate) != 0)
  {
    // Log error
    stringstream logmsg;
    logmsg << "Uncaught exception raised from plugin! Plugin library on path: " << libPath
           << " Task id: " << m_id
           << " Terminating task process...";
    LOG(logmsg.str(), grader::FATAL);
    
    // Set task state so it can be deleted
    set_state(state::INVALID);
   
    // Terminate process
    abort();
  }
  
  // Load library
  unique_ptr<dynamic::shared_lib> lib;
  try 
  {
    lib.reset(new dynamic::shared_lib{libPath});
  } 
  catch (const dynamic::shared_lib_load_failed& e) 
  {
    stringstream logmsg;
    logmsg << "Loading shared library on path: " << libPath << " failed. ";
    logmsg << "Reason: " << e.what() << " Task id: " << m_id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  
  // Construct grader object
  auto graderObj = lib->make_object<grader_base>(configuration::get_grader_name(graderInfo));
  if (!graderObj)
  {
    stringstream logmsg;
    logmsg << "Failed to create grader object from library."
           << " Library path: " << libPath
           << " Grader name: " << configuration::get_grader_name(graderInfo)
           << " Task id: " << m_id;
    LOG(logmsg.str(), grader::ERROR);
    set_state(state::INVALID);
    return;
  }
  graderObj->initialize(this);
  
  // Compile source
  set_state(state::COMPILING);
  string compilationErr;
  ostringstream formater;
  if (!graderObj->compile(compilationErr))
  {
    transform(compilationErr.begin(), compilationErr.end(), compilationErr.begin(), 
              [=](char x) { return ('\n'== x || '\r' == x) ? ' ' : x;});
    formater << "{\n\t\"STATE\" : \"COMPILE_ERROR\",\n\t\"MESSAGE\" : \"" << compilationErr << "\"\n}";
    auto jsonStr = move(formater.str());
    m_status.insert(m_status.begin(), jsonStr.cbegin(), jsonStr.cend());
    set_state(state::COMPILE_ERROR);
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
  m_status = jsonStr.c_str();
  set_state(task::state::FINISHED);
  
  // Restore default std::terminate_handler
  set_terminate(defaultHandler);
}

const char* task::status() const
{
  boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
  switch(m_state)
  {
    case state::INVALID:
      return "{ \"STATE\" : \"INVALID\" }";
    case state::WAITING:
      return "{ \"STATE\" : \"WAITING\" }";
    case state::COMPILING:
      return "{ \"STATE\" : \"COMPILING\" }";
    case state::RUNNING:
      return "{ \"STATE\" : \"RUNNING\" }";
    case state::TIME_LIMIT: 
      return "{ \"STATE\" : \"TIME_LIMIT\" }";
    case state::FINISHED:
    case state::COMPILE_ERROR:
      return m_status.c_str();
  }
  return "";
}

task* task::create_task(const char* fileName, std::size_t fnLen, const char* fileContent, 
                        std::size_t fcLen, const char* testsContent, std::size_t testsCLen)
{
  // Generate uuid
  auto uuid = boost::uuids::random_generator()();
  shm_test_vector tests(shm().get_segment_manager());
  auto memTimeLang = parse_tests(testsContent, testsCLen, tests);
  if (INVALID_TEST_ATTR == memTimeLang) return nullptr;
  return shm().construct<task>(boost::uuids::to_string(uuid).c_str())(fileName, fnLen, fileContent, 
                                                                      fcLen, move(tests), uuid, 
                                                                      get<0>(memTimeLang), get<1>(memTimeLang),
                                                                      get<2>(memTimeLang)
                                                                      );
}

bool task::is_valid_task_name(const char* name)
{
   stringstream interpreter;
   boost::uuids::uuid tmp;
   return interpreter<<name && 
           interpreter>>tmp && 
           interpreter.get() == stringstream::traits_type::eof();
}

task::test_attributes task::parse_tests(const char* testsContent, std::size_t testsCLen, task::shm_test_vector& tests)
{
  // Read xml into property tree
  using namespace boost::property_tree;
  ptree pt;
  istringstream testsInput(string(testsContent, testsContent + testsCLen));
  try 
  {
    xml_parser::read_xml(testsInput, pt, xml_parser::no_comments);
  } 
  catch (const xml_parser::xml_parser_error& e) 
  {
    stringstream logmsg;
    logmsg << "Couldn't parse tests content (check if tests are xml valid). "
           << "Error message: " << e.what();
    LOG(logmsg.str(), grader::ERROR);
    return INVALID_TEST_ATTR;
  }
  
  // Get memory and time requirements from xml root element 'test'
  auto rootOpt = pt.get_child_optional("test");
  if (!rootOpt)
  {
    LOG("Bad config.xml file, there should be root element named 'test'.", grader::ERROR);
    return INVALID_TEST_ATTR;
  }
  auto root = *rootOpt;
  size_t memoryBytes = root.get<size_t>("<xmlattr>.memory", 0);
  size_t timeMilliseconds = root.get<size_t>("<xmlattr>.time", 0);
  string language = root.get<string>("<xmlattr>.language", "");
  if (0 == memoryBytes)
  {
    LOG("No memory constraint as attribute in 'test' element.", grader::ERROR);
    return INVALID_TEST_ATTR;
  }
  if (0 == timeMilliseconds)
  {
    LOG("No time constraint as attribute in 'test' element.", grader::ERROR);
    return INVALID_TEST_ATTR;
  }
  if ("" == language)
  {
    LOG("No programming language specified as attribute in 'test' element.", grader::ERROR);
    return INVALID_TEST_ATTR;
  }
  
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
      LOG("Invalid xml format! Expected 'input'!", grader::ERROR);
      return INVALID_TEST_ATTR;
    }
    else 
    {
      // Get input test
      subtest in = subtest(subtest::subtest_in, treeItBegin->second.data(), 
                        subtest::io_from_str(treeItBegin->second.get<string>("<xmlattr>.type", "std")),
                        treeItBegin->second.get("<xmlattr>.path", ""));
      
      // Advance to next xml element
      ++treeItBegin;
      if (treeItBegin == treeItEnd)
      {
        LOG("Invalid xml format! Expected 'output' element after 'input'!", grader::ERROR);
        return INVALID_TEST_ATTR;
      }
      if ("output" != treeItBegin->first)
      {
        LOG("Invalid xml format! Expected 'output' element!", grader::ERROR);
        return INVALID_TEST_ATTR; 
      }
      
      // Get output test, add new element to tests vector and advance in tree
      subtest out = subtest(subtest::subtest_out, treeItBegin->second.data(),
                      subtest::io_from_str(treeItBegin->second.get<string>("<xmlattr>.type", "std")),
                      treeItBegin->second.get("<xmlattr>.path", ""));
      tests.emplace_back(move(in), move(out));
    }
    ++treeItBegin;
  }
  
  return move(make_tuple(memoryBytes, timeMilliseconds, language));
}

void task::terminate_handler()
{
  longjmp(g_saveStateBeforeTerminate, 1);
}

void task::set_state(task::state newState)
{
  boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
  m_state = newState;
}

task::state task::get_state() const
{
  boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
  return m_state;
}
