// Project headers
#include "mod_grader.hpp"

// STL headers
#include <cstring>
#include <cctype>
#include <iostream>

// BOOST headers
#include <boost/algorithm/string.hpp>

namespace grader 
{
  int read_body(request_rec *r, const char **rbuf, apr_off_t *size)
  {
    /*~~~~~~~~*/
      int rc = OK;
      /*~~~~~~~~*/

      if((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))) {
          return(rc);
      }

      if(ap_should_client_block(r)) {

          /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
          char         argsbuffer[HUGE_STRING_LEN];
          apr_off_t    rsize, len_read, rpos = 0;
          apr_off_t length = r->remaining;
          /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

          *rbuf = (const char *) apr_pcalloc(r->pool, (apr_size_t) (length + 1));
          *size = length;
          
          // We will not read body greater than limit 
          if (static_cast<grader::limits::size_type>(length) > grader::limits::MAX_BODY_LENGTH)
            return (HTTP_REQUEST_ENTITY_TOO_LARGE);
          
          while((len_read = ap_get_client_block(r, argsbuffer, sizeof(argsbuffer))) > 0) {
              if((rpos + len_read) > length) {
                  rsize = length - rpos;
              }
              else {
                  rsize = len_read;
              }

              memcpy((char *) *rbuf + rpos, argsbuffer, (size_t) rsize);
              rpos += rsize;
          }
      }
      return(rc);
  }

  char* boundary(request_rec* r)
  {
    // Initialize apache api for getting request header
    auto fields = apr_table_elts(r->headers_in);
    auto e = reinterpret_cast<apr_table_entry_t*>(fields->elts);
    
    // Try to find Content-Type entry in header
    int i = 0;
    for(; i < fields->nelts; i++) 
      if (boost::iequals("CONTENT-TYPE", e[i].key))
        break;
    
    // Check why we exited loop (did we found Content-Type in header?)
    if (i != fields->nelts)
    {
      // Create new value string but without spaces
      auto valLen = std::strlen(e[i].val);
      std::size_t newValLen = 0U;
      char* noSpaceVal = static_cast<char*>(apr_palloc(r->pool, valLen + 1));
      std::copy_if(e[i].val, e[i].val + valLen, noSpaceVal, [&](char x) 
      {  
        if (!std::isspace(x))
        {
          ++newValLen;
          return true;
        }
        else return false;
      });
      
      // Case insensitive find of 'boundary=' string
      if (!newValLen) return nullptr;
      noSpaceVal[newValLen] = '\0';
      auto range = boost::ifind_first(noSpaceVal, "boundary=");
      if (range.empty()) return nullptr;
      
      // Construct return string
      auto retStrLen = (noSpaceVal + newValLen) - range.end();
      std::copy_n(range.end(), retStrLen, noSpaceVal);
      noSpaceVal[retStrLen] = '\0';
      return noSpaceVal;
    }
    else return nullptr;
  }
}

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
    ap_rprintf(r, "Results seeking request!\n%s", grader::boundary(r));
  }
  else if (r->method_number == M_POST)
  {
    // TODO: Grade files provided by POST method
    ap_set_content_type(r, "text/html");
    ap_rprintf(r, "Grading request!\n%s", grader::boundary(r));
  }
  else 
  {
    // TODO: Handle this error request!
    return (HTTP_NOT_FOUND);
  }
  return OK;
}