// Project headers
#include "http_tester.hpp"

// STL headers
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

// BOOST headers
#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// Testing headers
#include "../mod_grader/include/core/task.hpp"

using namespace std;
using namespace grader_test;
using namespace boost::property_tree;

const std::string base_dir = "../../mod_grader/src/examples/c";
http_tester tester("localhost", base_dir, "text/x-csrc");

inline void test_standard_case(const string& srcName, const string& testName, const ptree& correctResult)
{
  // Submit task and check that we got valid task id
  string taskId = tester.submit(srcName, testName);
  BOOST_CHECK(grader::task::is_valid_task_name(taskId.c_str()));
  
  // Pool for finished task
  ptree taskResult;
  do {
    // Wait a little
    this_thread::sleep_for(chrono::milliseconds(300));
    
    // Get task status
    string body = tester.fetch_status(taskId);
    istringstream taskStatusStream(body);
    json_parser::read_json(taskStatusStream, taskResult);
    string taskState = taskResult.get<string>("STATE");
    
    // Analyze task status
    if ("FINISHED" == taskState)
      break;
    else if ("INVALID" == taskState)
      throw runtime_error("Task state is INVALID!");
    else if ("COMPILE_ERROR" == taskState)
      throw runtime_error("Task state is COMPILE_ERROR!");
    taskResult.clear();
  } while (true);
  
  // Check task result
  BOOST_CHECK(correctResult == taskResult);
  
  // Delete finished task
  istringstream deletitionResultStream(tester.delete_task(taskId));
  taskResult.clear();
  json_parser::read_json(deletitionResultStream, taskResult);
  ptree destroyed;
  destroyed.put("STATE", "DESTROYED");
  BOOST_CHECK(destroyed == taskResult);
}

BOOST_AUTO_TEST_SUITE( Language_C )

BOOST_AUTO_TEST_CASE( std_std )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  
  test_standard_case("std_std.c", "std_std.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( std_file )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  
  test_standard_case("std_file.c", "std_file.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( cmd_std )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  
  test_standard_case("cmd_std.c", "cmd_std.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( cmd_file )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  
  test_standard_case("cmd_file.c", "cmd_file.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( file_std )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  
  test_standard_case("file_std.c", "file_std.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( file_file )
{
  ptree correctResult;
  correctResult.put("STATE", "FINISHED");
  correctResult.put("TEST0", "1");
  correctResult.put("TEST1", "1");
  correctResult.put("TEST2", "1");
  correctResult.put("TEST3", "1");
  
  test_standard_case("file_file.c", "file_file.xml", correctResult);
}

BOOST_AUTO_TEST_CASE( compiler_err )
{
    // Submit task and check that we got valid task id
  string taskId = tester.submit("compiler_err.c", "compiler_err.xml");
  BOOST_CHECK(grader::task::is_valid_task_name(taskId.c_str()));
  
  // Pool for finished task
  ptree taskResult;
  do {
    // Wait a little
    this_thread::sleep_for(chrono::milliseconds(300));
    
    // Get task status
    string body = tester.fetch_status(taskId);
    istringstream taskStatusStream(body);
    json_parser::read_json(taskStatusStream, taskResult);
    string taskState = taskResult.get<string>("STATE");
    
    // Analyze task status
    if ("FINISHED" == taskState)
      throw runtime_error("Expected COMPILE_ERROR, got FINISHED (task status).");
    else if ("INVALID" == taskState)
      throw runtime_error("Task state is INVALID!");
    else if ("COMPILE_ERROR" == taskState)
      break;
    taskResult.clear();
  } while (true);
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()