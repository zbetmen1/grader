// Project headers
#include "task.hpp"
#include "grader_base.hpp"
#include "dynamic/shared_lib.hpp"
#include "shared_memory.hpp"
#include "log.hpp"
#include "plugin_manager.hpp"
#include "autocall.hpp"
#include "configuration.hpp"

// STL headers
#include <algorithm>
#include <utility>
#include <sstream>
#include <string>
#include <functional>
#include <fstream>
#include <cstring>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/uuid/uuid.hpp>            
#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/iostreams/device/mapped_file.hpp>

// Unix headers
#include <unistd.h>

using namespace std;
using namespace grader;

const task::test_attributes task::invalid_test_attr{0, 0, ""};
const string task::source_file_name_base = "source.";
const string task::test_file_name = "tests.xml";

task::task(const char* fileName, std::size_t fnLen, const char* fileContent, std::size_t fcLen, const char* testsContents, 
           std::size_t testsCLen, const boost::uuids::uuid& id)
: m_state(state::WAITING), m_status(shared_memory::instance().get_segment_manager())
{
  // Initialize uuid
  const string strUUID = boost::lexical_cast<std::string>(id);
  copy(strUUID.cbegin(), strUUID.cend(), m_id);
  m_id[strUUID.size()] = '\0';
  
  // Construct source file name
  fs::path fname(fileName, fileName + fnLen);
  string sourceFileName = source_file_name_base + fname.extension().string();
  
  // Construct task directory, source and test file paths
  fs::path taskDir = dir_path();
  fs::path sourcePath = taskDir / sourceFileName;
  fs::path testPath = taskDir / test_file_name;
  
  // Create vector of cleanup autocalls
  vector<autocall> cleanups;
  cleanups.reserve(4);
  
  // Create directory for the task files
  fs::create_directories(taskDir);
  cleanups.emplace_back([&](){ fs::remove_all(taskDir); });
  
  // Zero async_io_req array and sig_event
  ::memset(static_cast<void*>(m_req), 0, sizeof(m_req));
  ::memset(static_cast<void*>(m_event), 0, sizeof(m_event));
  
  // Open file descriptors for writing
  int sourceFd, testFd;
  if ((sourceFd = ::open(sourcePath.c_str(), O_WRONLY)) == -1)
    throw task_exception(::strerror(errno));
  cleanups.emplace_back([=]() { ::close(sourceFd); });
  if ((testFd = ::open(testPath.c_str(), O_WRONLY)) == -1)
    throw task_exception(::strerror(errno));
  cleanups.emplace_back([=]() { ::close(testFd); });
  
  // Initialize buffer for array of async_io_req
  byte* reqBuff = new byte[fcLen + testsCLen];
  cleanups.emplace_back([=](){ delete[] reqBuff;});
  ::memcpy(reqBuff, fileContent, fcLen);
  ::memcpy(reqBuff + fcLen, testsContents, testsCLen);
  
  // Prepare I/O requests and sig_event member
  prepare_async_io_req(m_req[0], sourceFd, reqBuff, fcLen, LIO_WRITE);
  prepare_async_io_req(m_req[1], testFd, reqBuff + fcLen, testsCLen, LIO_WRITE);
  prepare_sig_event();
  
  // Launch asynchronous I/O requests
  if (::lio_listio(LIO_NOWAIT, m_req, 2, &m_event) == -1)
    throw task_exception(::strerror(errno));
  
  // Reset cleanups
  for_each(begin(cleanups), end(cleanups), [=](autocall& call) { call.release(); });
}

task::task(task&& oth)
:m_state(oth.m_state), m_status(boost::move(oth.m_status))
{
  copy(oth.m_id, oth.m_id + uuid_len, m_id);
  copy(oth.m_req, oth.m_req + 2, m_req);
  m_event = move(oth.m_event);
  m_ready = boost::move(oth.m_ready);
  m_lock = boost::move(oth.m_lock);
}

task& task::operator=(task&& oth)
{
  if (&oth != this)
  {
    // Copy other task
    copy(oth.m_id, oth.m_id + uuid_len, m_id);
    m_state = oth.m_state;
    m_status = boost::move(oth.m_status);
    copy(oth.m_req, oth.m_req + 2, m_req);
    m_event = oth.m_event;
    m_ready = boost::move(oth.m_ready);
    m_lock = boost::move(oth.m_lock);
    
    // Clear other task data
    ::memset(oth.m_id, 0, uuid_len);
    oth.m_state = state::INVALID;
    ::memset(oth.m_req, 0, sizeof(oth.m_req));
    ::memset(oth.m_event, 0, sizeof(oth.m_event));
  }
  return *this;
}

task::~task()
{
  fs::remove_all(dir_path());
  wait_for_disk_flush_to_complete();
}

void task::status(string& fillStatus) const
{
  shared_memory& shm = shared_memory::instance();
  bool deleteMe = false;
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
  switch(m_state)
  {
    case state::INVALID:
      fillStatus = "{ \"STATE\" : \"INVALID\" }";
      deleteMe = true;
      break;
    case state::WAITING:
      fillStatus = "{ \"STATE\" : \"WAITING\" }";
      break;
    case state::COMPILING:
      fillStatus = "{ \"STATE\" : \"COMPILING\" }";
      break;
    case state::RUNNING:
      fillStatus = "{ \"STATE\" : \"RUNNING\" }";
      break;
    case state::FINISHED:
    case state::COMPILE_ERROR:
      fillStatus = m_status.c_str();
      deleteMe = true;
      break;
  }
  
  if (deleteMe) shm.destroy_ptr<task>(this);
}

task::state task::get_state() const
{
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
  return m_state;
}

void task::set_state(task::state newState)
{
  boost::interprocess::scoped_lock<mutex_type> lock(m_lock);
  m_state = newState;
}

fs::path task::dir_path() const
{
  const configuration& conf = configuration::instance();
  fs::path taskDir = conf.get(configuration::source_base_dir);
  taskDir /= m_id;
  return boost::move(taskDir);
}

void task::run_all(int workerIdx)
{ 
  // Safety
  autocall invalidIfException([=](){ set_state(state::INVALID); });
  
  // Parse tests
  vector<test> tests;
  auto memTimeLang = parse_tests(read_from_disk(dir_path() + '/' + test_file_name) , tests);
//   size_t memory = get<0>(memTimeLang);
//   size_t time = get<0>(memTimeLang);
  string language = get<2>(memTimeLang);
  
  // Fetch grader informations for programming language
  plugin_manager& plmgr = plugin_manager::instance();
  auto graderInfo = plmgr.get(language);
  
  // Construct grader object
  auto graderObj = graderInfo.lib.make_object<grader_base>(graderInfo.class_name);
  if (!graderObj)
  {
    glog::error() << "Failed to create grader object from library."
                  << " Grader name: " << graderInfo.class_name
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
  
  // Release safety
  invalidIfException.release();
}

void task::wait_for_disk_flush_to_complete()
{
  m_ready.wait([=](){ return m_req[0].aio_buf == nullptr; });
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
    return invalid_test_attr;
  }
  
  // Get memory and time requirements from xml root element 'test'
  auto rootOpt = pt.get_child_optional("test");
  if (!rootOpt)
  {
    glog::error() << "Bad config.xml file, there should be root element named 'test'.\n";
    return invalid_test_attr;
  }
  auto root = *rootOpt;
  size_t memoryBytes = root.get<size_t>("<xmlattr>.memory", 0);
  size_t timeMilliseconds = root.get<size_t>("<xmlattr>.time", 0);
  string language = root.get<string>("<xmlattr>.language", "");
  if (0 == memoryBytes)
  {
    glog::error() << "No memory constraint as attribute in 'test' element.\n";
    return invalid_test_attr;
  }
  if (0 == timeMilliseconds)
  {
    glog::error() << "No time constraint as attribute in 'test' element.\n";
    return invalid_test_attr;
  }
  if ("" == language)
  {
    glog::error() << "No programming language specified as attribute in 'test' element.\n";
    return invalid_test_attr;
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
      return invalid_test_attr;
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
        return invalid_test_attr;
      }
      if ("output" != treeItBegin->first)
      {
        glog::error() << "Invalid xml format! Expected 'output' element!\n";
        return invalid_test_attr; 
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

void task::prepare_async_io_req(async_io_req& req, int filedes, void* buff, size_t nbytes, int opcode)
{
  req.aio_fildes = filedes;
  req.aio_buf = buff;
  req.aio_nbytes = nbytes;
  req.aio_lio_opcode = opcode;
}

void task::prepare_sig_event()
{
  m_event.sigev_notify = SIGEV_THREAD;
  m_event.notify_function = &handle_disk_flush_done;
  m_event.notify_attributes = nullptr;
  m_event.sigev_value.sival_ptr = this;
}

void task::handle_disk_flush_done(sig_val sv)
{
  // Get task ptr
  task* thisTask = reinterpret_cast<task*>(sv.sival_ptr);
  
  // Free allocated buffer
  delete[] thisTask->m_req[0].aio_buf;
  thisTask->m_req[0].aio_buf = nullptr;
  
  // Notify all processes that disk flush is done (successful of not)
  thisTask->m_ready.notify_all();
  
  // TODO: Handle errors
  int errorCode;
  if (!(errorCode = ::aio_error(thisTask->m_req)))
    throw task_exception(::strerror(errorCode));
  if (!(errorCode = ::aio_error(thisTask->m_req + 1)))
    throw task_exception(::strerror(errorCode));
}

