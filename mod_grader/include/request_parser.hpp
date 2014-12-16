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

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

// STL headers
#include <cstddef>
#include <tuple>

struct request_rec;

namespace grader
{ 
  class request_parser
  {
  public:
    // Types and constants
    using size_type = std::size_t;
    using parsed_data = std::tuple<char*       /* file name */, 
                                   std::size_t /* file name length */, 
                                   char*       /* file content */,
                                   std::size_t /* file content length */,
                                   char*       /* tests */,
                                   std::size_t /* tests length */
                                   >;
    static constexpr unsigned char FILE_NAME = 0;
    static constexpr unsigned char FILE_NAME_LEN = 1;
    static constexpr unsigned char FILE_CONTENT = 2;
    static constexpr unsigned char FILE_CONTENT_LEN = 3;
    static constexpr unsigned char TESTS_CONTENT = 4;
    static constexpr unsigned char TESTS_CONTENT_LEN = 5;
    
    static constexpr size_type MAX_BODY_LENGTH = (1U << 20) * 20; // 20MB
  private:
    request_rec* m_r;
  public:
    explicit request_parser(request_rec* r);
    
    int parse(grader::request_parser::parsed_data& toFill) const;
  private:
  /**
    * @brief Reads whole body of request but not header.
    * @details Body content is put to rbuf and number of bytes in body is kept in size.
    * Function returns HTTP codes to indicate success and failure.
    * 
    * @param r Current request.
    * @param rbuf Body buffer.
    * @param size Number of bytes that body has.
    * @return int
    */
    int read_body(char*& rbuf, std::size_t& rbufLen) const;

  /**
    * @brief Gets boundary for POST request with multipart/form-data encoding.
    * @details This function works correct but expects request header to have specific format.
    * It expects line like: "Content-Type: multipart/form-data; boundary=---------------------------33865453060129036431648023".
    * What is important here is:
    *   1) case doesn't matter (upper or lower or mixed)
    *   2) Content-Type line must exist in header
    *   3) function doesn't care what's between 'Content-Type' and 'boundary'
    *   4) boundary must come last, for this example: 
    *      "Content-Type: boundary=---------------------------33865453060129036431648023; multipart/form-data"
    *      function will not work as expected.
    * Note that 4th requirement is fulfilled at the time code was written but is given here so errors could
    * be fixed easy if format ever changes.
    * 
    * @param r Current request.
    * @return char* Boundary that can be used to split request body.
    */
    char* get_boundary(size_t& boundaryLen) const;
  };
}

#endif // REQUEST_PARSER_H
