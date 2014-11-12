/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2014  Kocic Ognjen <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Project headers
#include "request_parser.hpp"

// STL headers
#include <algorithm>

// BOOST headers
#include <boost/algorithm/string.hpp>

// Apache core headers
#include <httpd.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_request.h>
#include <apr_strings.h>

using namespace std;

namespace grader
{
  request_parser::request_parser(request_rec* r)
  : m_r(r)
  {
  }

  int request_parser::parse(request_parser::parsed_data& toFill) const
  {
    // Read boundary and body
    size_t boundaryLen, bodyLen;
    char* boundary,* body;
    boundary = get_boundary(boundaryLen);
    int httpCode = read_body(body, bodyLen);
    if (!boundary)
      return (HTTP_BAD_REQUEST);
    if ((OK) != httpCode)
      return httpCode;
    
    // Example of body starts with next three lines:
    //   -----------------------------75180758514773109461831266709 /*this is boundary*/
    //   Content-Disposition: form-data; name="fileToUpload"; filename="main.cpp" 
    //   Content-Type: text/x-c++src
    // To parse data we can skip first 'boundaryLen' bytes and then extract first line.
    // From that line we want to fetch value of filename. Next, we get to the last line
    // and move past it. Until we reach a new boundary we read all that data as a file data.
    
    // Skip boundary
    body += boundaryLen;
    while (*body == '\n' || *body == '\r') ++body;
    
    // Find file name attribute
    auto range = boost::ifind_first(body, "filename=\"");
    if (range.empty()) return (HTTP_BAD_REQUEST);
    
    // Initialize file name and size
    char* filename = range.end();
    get<SRC_NAME>(toFill) = filename;
    while (*filename != '\"') ++filename;
    get<SRC_SIZE>(toFill) = static_cast<size_t>(filename - range.end());
    
    // Skip two lines (this line and Content-Type line)
    body = filename + 1;
    while (*body != '\n' && *body != '\r') ++body; // Move to the end of line
    while (*body == '\n' || *body == '\r') ++body; // Move to next line skipping new line chars (skipped FIRST line at the end)
    while (*body != '\n' && *body != '\r') ++body; // Move to the end of line
    while (*body == '\n' || *body == '\r') ++body; // Move to next line skipping new line chars (skipped TWO lines in total)
    
    // Find next boundary, that will be end of file. File begins at 'body'
    get<SRC_CONTENT>(toFill) = body;
    range = boost::find_first(body, boundary);
    get<SRC_CONTENT_LEN>(toFill) = range.begin() - get<SRC_CONTENT>(toFill);
    
    // Move to the next line
    body = range.end();
    while (*body == '\n' || *body == '\r') ++body;
    
    // Skip 2 lines
    while (*body != '\n' && *body != '\r') ++body; // Move to the end of line
    while (*body == '\n' || *body == '\r') ++body; // Move to next line skipping new line chars (skipped FIRST line at the end)
    while (*body != '\n' && *body != '\r') ++body; // Move to the end of line
    while (*body == '\n' || *body == '\r') ++body; // Move to next line skipping new line chars (skipped TWO lines in total)
    
    // Find next boundary, content of tests.xml begins at 'body'
    get<TESTS_CONTENT>(toFill) = body;
    range = boost::find_first(body, boundary);
    get<TESTS_CONTENT_LEN>(toFill) = range.begin() - get<TESTS_CONTENT>(toFill);
    return 0;
  }
  
  int request_parser::read_body(char*& rbuf, std::size_t& rbufLen) const
  {
    /*~~~~~~~~*/
      int rc = OK;
      /*~~~~~~~~*/

      if((rc = ap_setup_client_block(m_r, REQUEST_CHUNKED_ERROR))) {
          return(rc);
      }

      if(ap_should_client_block(m_r)) {

          /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
          char         argsbuffer[HUGE_STRING_LEN];
          apr_off_t    rsize, len_read, rpos = 0;
          apr_off_t length = m_r->remaining;
          /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

          rbuf = static_cast<char*>(apr_pcalloc(m_r->pool, (apr_size_t) (length + 1)));
          rbufLen = length;
          
          // We will not read body greater than limit 
          if (static_cast<grader::request_parser::size_type>(length) > grader::request_parser::MAX_BODY_LENGTH)
            return (HTTP_REQUEST_ENTITY_TOO_LARGE);
          
          while((len_read = ap_get_client_block(m_r, argsbuffer, sizeof(argsbuffer))) > 0) 
          {
              if((rpos + len_read) > length) 
              {
                  rsize = length - rpos;
              }
              else 
              {
                  rsize = len_read;
              }

              memcpy(rbuf + rpos, argsbuffer, (size_t) rsize);
              rpos += rsize;
          }
      }
      return(rc);
  }

  char* request_parser::get_boundary(size_t& boundaryLen) const
  {
    // Initialize apache api for getting request header
    auto fields = apr_table_elts(m_r->headers_in);
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
      char* noSpaceVal = static_cast<char*>(apr_palloc(m_r->pool, valLen + 1));
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
      
      // Construct return string and note the danger. Browsers 
      // append '--' at the beginning of boundary so that must be appended
      // to return string. This was noted on Mozzila Firefox and Google Chrome
      // browsers and should be checked to see if that's a standard way to do it.
      auto retStrLen = (noSpaceVal + newValLen) - range.end();
      std::copy_n(range.end(), retStrLen, noSpaceVal + 2);
      noSpaceVal[0] = '-';
      noSpaceVal[1] = '-';
      noSpaceVal[retStrLen + 2] = '\0';
      boundaryLen = retStrLen + 2;
      return noSpaceVal;
    }
    else return nullptr;
  }
}