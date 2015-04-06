// Project headers
#include "task.hpp"
#include "configuration.hpp"
#include "grader_base.hpp"
#include "dynamic/shared_lib.hpp"
#include "shared_memory.hpp"
#include "log.hpp"

// STL headers
#include <algorithm>
#include <utility>
#include <sstream>
#include <string>
#include <functional>
#include <csetjmp>
#include <fstream>

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
#include <boost/iostreams/device/mapped_file.hpp>

using namespace std;
using namespace grader;

const task::test_attributes task::INVALID_TEST_ATTR{0, 0, ""};
const char* task::TESTS_FILE_NAME = "tests.xml";

jmp_buf g_saveStateBeforeTerminate;

task::task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, const char* testsContents, 
           std::size_t testsCLen, const boost::uuids::uuid& id)
: m_fileName(shared_memory::instance().get_segment_manager()), m_state(state::WAITING), m_status(shared_memory::instance().get_segment_manager())
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
    glog::error() << "Invalid file name! Task id: " << id << '\n';
    set_state(state::INVALID);
    return;
  }
  if (tmpPath.has_filename())
  {
    m_fileName = tmpPath.filename().c_str();
  }
  else 
  {
    glog::error() << "Bad file name in task constructor: " << tmpPath.c_str() << " task id: " << id << '\n';
    set_state(state::INVALID);
    return;
  }
  
  // Copy uuid
  const string tmp = boost::lexical_cast<std::string>(id);
  copy(tmp.cbegin(), tmp.cend(), m_id);
  m_id[tmp.size()] = '\0';
  
  // Create task directory
  boost::system::error_code code;
  string dirPath = dir_path();
  boost::filesystem::create_directories(dirPath, code);
  if (boost::system::errc::success != code)
  {
    glog::error() << "Error when creating directory: " << dirPath
           << " Message: " << code.message()
           << " Id: " << m_id << '\n';
    set_state(state::INVALID);
    return;
  }
  
  // Write source to disk
  string srcContent{fileContent, fileContent + fcLen};
  write_to_disk(source_path(), srcContent);
  
  // Write test file to disk
  string testsContent{testsContents, testsContents + testsCLen};
  write_to_disk(dirPath + '/' + TESTS_FILE_NAME, testsContent);
}

task::~task()
{
  boost::system::error_code code;
  string dirPath = dir_path();
  boost::filesystem::remove_all(dirPath, code);
  if (boost::system::errc::success != code)
  {
    glog::error() << "Error when removing directory: " << dirPath
           << " Message: " << code.message()
           << " Id: " << m_id << '\n';
  }
}

task::task(task&& oth)
: m_fileName(boost::move(oth.m_fileName)), m_state(oth.m_state), m_status(boost::move(oth.m_status))
{
  copy(oth.m_id, oth.m_id + UUID_BYTES, m_id);
}

task& task::operator=(task&& oth)
{
  if (&oth != this)
  {
    m_fileName = boost::move(oth.m_fileName);
    copy(oth.m_id, oth.m_id + UUID_BYTES, m_id);
    m_state = oth.m_state;
    m_status = boost::move(oth.m_status);
  }
  return *this;
}

void task::run_all()
{
  // Parse tests
  vector<test> tests;
  auto memTimeLang = parse_tests(read_from_disk(dir_path() + '/' + TESTS_FILE_NAME) , tests);
//   size_t memory = get<0>(memTimeLang);
//   size_t time = get<0>(memTimeLang);
  string language = get<2>(memTimeLang);
  
  // Fetch grader informations for programming language
  const configuration& conf = configuration::instance();
  auto graderInfo = conf.get_grader(language);
  
  // Check if grader for this language doesn't exists in config.xml
  if (configuration::INVALID_GR_INFO == graderInfo)
  {
    glog::error() << "Couldn't find grader for language: " << language << " in config.xml."
                  << "Task id: " << m_id << '\n';
    set_state(state::INVALID);
    return;
  }
  
  // Get base path where all grader libraries are (check that lib dir is set in config.xml)
  auto baseLibPathIt = conf.get(configuration::LIB_DIR);
  if (conf.invalid() == baseLibPathIt)
  {
    glog::error() << "Couldn't find entry for directory where all grader libraries are in config.xml."
                  << "Task id: " << m_id << '\n';
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
    glog::fatal() << "Uncaught exception raised from plugin! Plugin library on path: " << libPath
                  << " Task id: " << m_id
                  << " Terminating task process...\n";
    
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
    glog::error() << "Loading shared library on path: " << libPath << " failed. "
           << "Reason: " << e.what() << " Task id: " << m_id << '\n';
    set_state(state::INVALID);
    return;
  }
  
  // Construct grader object
  auto graderObj = lib->make_object<grader_base>(configuration::get_grader_name(graderInfo));
  if (!graderObj)
  {
    glog::error() << "Failed to create grader object from library."
                  << " Library path: " << libPath
                  << " Grader name: " << configuration::get_grader_name(graderInfo)
                  << " Task id: " << m_id << '\n';
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
  testResults.reserve(tests.size());
  for (const auto& t : tests)
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
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
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
    case state::FINISHED:
    case state::COMPILE_ERROR:
      return m_status.c_str();
  }
  return "";
}

task::state task::get_state() const
{
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
  return m_state;
}

task* task::create_task(const char* fileName, std::size_t fnLen, const char* fileContent, 
                        std::size_t fcLen, const char* testsContent, std::size_t testsCLen)
{
  // Generate uuid and create task
  auto uuid = boost::uuids::random_generator()();
  return shared_memory::instance().construct<task>(boost::uuids::to_string(uuid).c_str(), fileName, fnLen, fileContent, 
                                                                      fcLen, testsContent, testsCLen, uuid);
}

bool task::is_valid_task_name(const char* name)
{
   stringstream interpreter;
   boost::uuids::uuid tmp;
   return interpreter<<name && 
           interpreter>>tmp && 
           interpreter.get() == stringstream::traits_type::eof();
}

task::test_attributes task::parse_tests(const string& testsStr, vector<test>& tests)
{
  // Read xml into property tree
  using namespace boost::property_tree;
  ptree pt;
  istringstream testsInput(testsStr);
  try 
  {
    xml_parser::read_xml(testsInput, pt, xml_parser::no_comments);
  } 
  catch (const xml_parser::xml_parser_error& e) 
  {
    glog::error() << "Couldn't parse tests content (check if tests are xml valid). "
           << "Error message: " << e.what() << '\n';
    return INVALID_TEST_ATTR;
  }
  
  // Get memory and time requirements from xml root element 'test'
  auto rootOpt = pt.get_child_optional("test");
  if (!rootOpt)
  {
    glog::error() << "Bad config.xml file, there should be root element named 'test'.\n";
    return INVALID_TEST_ATTR;
  }
  auto root = *rootOpt;
  size_t memoryBytes = root.get<size_t>("<xmlattr>.memory", 0);
  size_t timeMilliseconds = root.get<size_t>("<xmlattr>.time", 0);
  string language = root.get<string>("<xmlattr>.language", "");
  if (0 == memoryBytes)
  {
    glog::error() << "No memory constraint as attribute in 'test' element.\n";
    return INVALID_TEST_ATTR;
  }
  if (0 == timeMilliseconds)
  {
    glog::error() << "No time constraint as attribute in 'test' element.\n";
    return INVALID_TEST_ATTR;
  }
  if ("" == language)
  {
    glog::error() << "No programming language specified as attribute in 'test' element.\n";
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
      glog::error() << "Invalid xml format! Expected 'input'!\n";
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
        glog::error() << "Invalid xml format! Expected 'output' element after 'input'!\n";
        return INVALID_TEST_ATTR;
      }
      if ("output" != treeItBegin->first)
      {
        glog::error() << "Invalid xml format! Expected 'output' element!\n";
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
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
  m_state = newState;
}

string task::dir_path() const
{
  const configuration& conf = configuration::instance();
  auto baseDirIt = conf.get(configuration::BASE_DIR);
  if (conf.invalid() == baseDirIt)
  {
    return "";
  }
  
  auto dpath = baseDirIt->second + "/" + m_id;
  return move(dpath);
}

string task::source_path() const
{
  return dir_path() + "/" + m_fileName.c_str();
}

string task::executable_path() const
{
  return dir_path() + "/" + strip_extension(m_fileName.c_str());
}

string task::strip_extension(const string& fileName) const
{
  auto pointPos = fileName.find_last_of('.');
  return fileName.substr(0, pointPos);
}

string task::get_extension(const string& fileName) const
{
  auto pointPos = fileName.find_last_of('.');
  return fileName.substr(pointPos + 1);
}

void task::write_to_disk(const string& path, const string& content)
{
  boost::iostreams::mapped_file_params params;
  params.path = path;
  params.new_file_size = content.length();
  params.flags = boost::iostreams::mapped_file::mapmode::readwrite;
  boost::iostreams::mapped_file mf;
  mf.open(params);
  if (mf.is_open())
    copy(content.cbegin(), content.cend(), mf.data());
  else 
  {
    glog::error() << "Couldn't open memory mapped file for writing: " << path << '\n';
  }
}

string task::read_from_disk(const string& path)
{
  ifstream tests{path.c_str()};
  string content{istreambuf_iterator<char>{tests}, istreambuf_iterator<char>{}};
  return move(content);
}
