// Project headers
#include "mod_grader.hpp"
#include "request_parser.hpp"

// STL headers
#include <cstring>
#include <cctype>
#include <iostream>
#include <fstream>
#include <cerrno>

// BOOST headers
#include <boost/algorithm/string.hpp>

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
    string fileContent(get<request_parser::SRC_CONTENT>(data),
                       get<request_parser::SRC_CONTENT>(data) +
                       get<request_parser::SRC_CONTENT_LEN>(data)
                      );
    boost::replace_all(fileContent, "<", "&lt");
    boost::replace_all(fileContent, ">", "&gt");
    ap_set_content_type(r, "text/html");
    ap_rprintf(r, "<pre>%s</pre>\n", fileContent.c_str());
  }
  else 
  {
    // TODO: Handle this error request!
    return (HTTP_NOT_FOUND);
  }
  return OK;
}