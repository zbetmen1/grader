// Project headers
#include "mod_grader.hpp"
#include "request_parser.hpp"
#include "task.hpp"
#include "configuration.hpp"
#include "grader_log.hpp"
#include "interprocess_queue.hpp"
#include "grader_pool.hpp"
#include "shared_memory.hpp"

// STL headers
#include <cstring>
#include <cctype>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <string>
#include <cstddef>
#include <atomic>

// BOOST headers
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/filesystem.hpp>

// Apache headers
#include <apr_tables.h>

// Linux headers
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace grader;

template <typename T>
using shm_ptr = boost::interprocess::offset_ptr<T>;
using task_queue = interprocess_queue<shm_ptr<task>>;

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
    glog::error() << "Invalid file name from request. File name: " << r->filename
                  << " Error message: " << e.what() << '\n';
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
  grader_pool& GRADER_POOL = grader_pool::instance();
  /* Dispatch on type of request, when request type is POST
   assume that we have new grading to do, but if request type
   is GET assume that clients are asking for grading results */
  if (r->method_number == M_GET)
  {
    glog::debug() << "Accepted request; method: GET address: " << r->filename << '\n';
    char* taskId = task_id_from_url(r);
    if (taskId && task::is_valid_task_name(taskId))
    {
      auto foundTask = shared_memory::instance().find<task>(taskId);
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
    glog::debug() << "Accepted request; method: POST address: " << r->filename << '\n';
    request_parser parser(r);
    request_parser::parsed_data data;
    int httpCode = parser.parse(data);
    if (OK != httpCode)
    {
      glog::error() << "Bad POST request. Http code: " << httpCode << '\n';
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
      shared_memory::instance().destroy<task>(newTask->id());
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    glog::debug() << "Created task with id: "<< newTask->id() << '\n';
    GRADER_POOL.submit(newTask);
    ap_rprintf(r, "%s", newTask->id());
  }
  else if (r->method_number == M_DELETE)
  {
    glog::debug() << "Accepted request; method: DELETE address: " << r->filename << '\n';
    char* taskId = task_id_from_url(r);
    if (taskId && task::is_valid_task_name(taskId))
    {
      auto foundTask = shared_memory::instance().find<task>(taskId);
      if (foundTask && (task::state::FINISHED == foundTask->get_state() ||
                        task::state::COMPILE_ERROR == foundTask->get_state() ||
                        task::state::INVALID == foundTask->get_state()))
      {
        shared_memory::instance().destroy<task>(taskId);
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
    glog::warning() << "Unsupported request method; address: " << r->filename << '\n';
    return (HTTP_NOT_FOUND);
  }
  return OK;
}