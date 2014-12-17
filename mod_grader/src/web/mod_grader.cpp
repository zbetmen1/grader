// Project headers
#include "mod_grader.hpp"
#include "request_parser.hpp"
#include "task.hpp"

// STL headers
#include <cstring>
#include <cctype>
#include <iostream>
#include <fstream>
#include <cerrno>

// BOOST headers
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

using namespace std;
using namespace grader;

EXTERN_C void register_hooks(apr_pool_t* /*pool*/)
{
  ap_hook_handler(grader_handler, NULL, NULL, APR_HOOK_LAST);
}

EXTERN_C int grader_handler(request_rec* r)
{
  if (!r->handler || strcmp(r->handler, "grader")) return (DECLINED);
  
  // Dispatch on type of request, when request type is POST
  // assume that we have new grading to do, but if request type
  // is GET assume that clients are asking for grading results
  if (r->method_number == M_GET)
  {
    // TODO: Supply grading results
    ap_set_content_type(r, "text/html");
    ap_rprintf(r, "Results seeking request!\n");
  }
  else if (r->method_number == M_POST)
  {
    // TODO: Grade files provided by POST method
    request_parser parser(r);
    request_parser::parsed_data data;
    parser.parse(data);
    task* newTask = task::create_task(get<request_parser::FILE_NAME>(data),
                                      get<request_parser::FILE_NAME_LEN>(data),
                                      get<request_parser::FILE_CONTENT>(data),
                                      get<request_parser::FILE_CONTENT_LEN>(data),
                                      get<request_parser::TESTS_CONTENT>(data),
                                      get<request_parser::TESTS_CONTENT_LEN>(data));
    newTask->run_all();
    ap_rprintf(r, "%s sizeof(task) = %lu \n", newTask->id(), sizeof(task));
  }
  else if (r->method_number == M_DELETE)
  {
    
  }
  else 
  {
    // TODO: Handle this error request!
    return (HTTP_NOT_FOUND);
  }
  return OK;
}