// Project headers
#include "mod_grader.hpp"
#include "request_parser.hpp"
#include "task.hpp"
#include "configuration.hpp"
#include "grader_log.hpp"

// STL headers
#include <cstring>
#include <cctype>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <string>

// BOOST headers
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/filesystem.hpp>

// Apache headers
#include <apr_tables.h>

// Linux headers
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace grader;

EXTERN_C void register_hooks(apr_pool_t* /*pool*/)
{
  ap_hook_handler(grader_handler, NULL, NULL, APR_HOOK_LAST);
}

char* task_id_from_url(request_rec* r)
{
  // Check path
  boost::filesystem::path p;
  try
  {
    p = boost::filesystem::path(r->filename);
  } catch(const exception& e)
  {
    stringstream logmsg;
    logmsg << "Invalid file name from request. File name: " << r->filename
           << " Error message: " << e.what();
    LOG(logmsg.str(), grader::ERROR);
    return nullptr;
  }
  
  // Get file name
  const char* taskIdWithExt = p.filename().c_str();
  char* taskId = apr_pstrdup(r->pool, taskIdWithExt);
  auto len = strlen(taskId);
  taskId[len - 6] = '\0';
  return taskId;
}

EXTERN_C int grader_handler(request_rec* r)
{
  if (!r->handler || strcmp(r->handler, "grader")) return (DECLINED);
  
  // Set signal handler to avoid zombie processes
  signal(SIGCHLD, &avoid_zombie_handler);
  
  /* Dispatch on type of request, when request type is POST
   assume that we have new grading to do, but if request type
   is GET assume that clients are asking for grading results */
  if (r->method_number == M_GET)
  {
    LOG(apr_pstrcat(r->pool, "Accepted request; method: GET address: ", r->filename, nullptr), grader::DEBUG);
    char* taskId = task_id_from_url(r);
    if (taskId && task::is_valid_task_name(taskId))
    {
      auto foundTask = shm_find<task>(taskId);
      if (foundTask)
      {
        ap_rprintf(r, "%s", foundTask->status());
      }
      else 
      {
        ap_rprintf(r, "{ \"STATE\" : \"NOT_FOUND\" }");
      }
    }
    else 
    {
      ap_rprintf(r, "{ \"STATE\" : \"INVALID_TASK_NAME\" }");
    }
  }
  else if (r->method_number == M_POST)
  {
    LOG(apr_pstrcat(r->pool, "Accepted request; method: POST address: ", r->filename, nullptr), grader::DEBUG);
    request_parser parser(r);
    request_parser::parsed_data data;
    int httpCode = parser.parse(data);
    if (OK != httpCode)
    {
      stringstream logmsg;
      logmsg << "Bad POST request. Http code: " << httpCode;
      LOG(logmsg.str(), grader::ERROR);
      return httpCode;
    }
    task* newTask = task::create_task(get<request_parser::FILE_NAME>(data),
                                      get<request_parser::FILE_NAME_LEN>(data),
                                      get<request_parser::FILE_CONTENT>(data),
                                      get<request_parser::FILE_CONTENT_LEN>(data),
                                      get<request_parser::TESTS_CONTENT>(data),
                                      get<request_parser::TESTS_CONTENT_LEN>(data));
    if (!newTask || task::state::INVALID == newTask->get_state())
    {
      shm_destroy<task>(newTask->id());
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    LOG(apr_pstrcat(r->pool, "Created task with id: ", newTask->id(), nullptr), grader::DEBUG);
    int pid = fork();
    if (-1 == pid)
    {
      stringstream logmsg;
      logmsg << "Forking child process to do task failed! Error msg: "
             <<  strerror(errno);
      LOG(logmsg.str(), grader::ERROR);
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    // Child process
    if (0 == pid)
    {
      newTask->run_all();
      exit(EXIT_SUCCESS);
    }
    else 
    { // Parent process
      ap_rprintf(r, "%s", newTask->id());
    }
  }
  else if (r->method_number == M_DELETE)
  {
    LOG(apr_pstrcat(r->pool, "Accepted request; method: DELETE address: ", r->filename, nullptr), grader::DEBUG);
    char* taskId = task_id_from_url(r);
    if (taskId && task::is_valid_task_name(taskId))
    {
      auto foundTask = shm_find<task>(taskId);
      if (foundTask && (task::state::FINISHED == foundTask->get_state() ||
                        task::state::COMPILE_ERROR == foundTask->get_state() ||
                        task::state::INVALID == foundTask->get_state()))
      {
        shm_destroy<task>(taskId);
        ap_rprintf(r, "{ \"STATE\" : \"DESTROYED\" }");
      }
      else 
      {
        ap_rprintf(r, "{ \"STATE\" : \"NOT_FOUND\" }");
      }
    }
    else 
    {
      ap_rprintf(r, "{ \"STATE\" : \"INVALID_TASK_NAME\" }");
    }
  }
  else 
  {
    LOG(apr_pstrcat(r->pool, "Unsupported request method; address: ", r->filename, nullptr), grader::WARNING);
    return (HTTP_NOT_FOUND);
  }
  return OK;
}

void avoid_zombie_handler(int)
{
  int childStatus;
  wait(&childStatus);
}
