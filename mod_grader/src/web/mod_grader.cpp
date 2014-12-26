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
#include <unistd.h>

// BOOST headers
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/filesystem.hpp>

// Apache headers
#include <apr_tables.h>

using namespace std;
using namespace grader;

EXTERN_C void register_hooks(apr_pool_t* /*pool*/)
{
  ap_hook_handler(grader_handler, NULL, NULL, APR_HOOK_LAST);
}

char* task_id_from_url(request_rec* r)
{
  boost::filesystem::path p(r->filename);
  const char* taskIdWithExt = p.filename().c_str();
  char* taskId = apr_pstrdup(r->pool, taskIdWithExt);
  auto len = strlen(taskId);
  taskId[len - 6] = '\0';
  return taskId;
}

EXTERN_C int grader_handler(request_rec* r)
{
  if (!r->handler || strcmp(r->handler, "grader")) return (DECLINED);
  
  /* Dispatch on type of request, when request type is POST
   assume that we have new grading to do, but if request type
   is GET assume that clients are asking for grading results */
  if (r->method_number == M_GET)
  {
    LOG(apr_pstrcat(r->pool, "Accepted request; method: GET address: ", r->filename, nullptr), grader::DEBUG);
    char* taskId = task_id_from_url(r);
    if (task::is_valid_task_name(taskId))
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
    parser.parse(data);
    task* newTask = task::create_task(get<request_parser::FILE_NAME>(data),
                                      get<request_parser::FILE_NAME_LEN>(data),
                                      get<request_parser::FILE_CONTENT>(data),
                                      get<request_parser::FILE_CONTENT_LEN>(data),
                                      get<request_parser::TESTS_CONTENT>(data),
                                      get<request_parser::TESTS_CONTENT_LEN>(data));
    LOG(apr_pstrcat(r->pool, "Created task with id: ", newTask->id(), nullptr), grader::DEBUG);
    int pid = fork();
    
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
    if (task::is_valid_task_name(taskId))
    {
      auto foundTask = shm_find<task>(taskId);
      if (foundTask && (task::state::FINISHED == foundTask->get_state() ||
                        task::state::COMPILE_ERROR == foundTask->get_state()))
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