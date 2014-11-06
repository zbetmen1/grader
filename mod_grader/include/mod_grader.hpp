#ifndef MOD_GRADER_H
#define MOD_GRADER_H

// STL headers
#include <cstddef>

// Apache core headers
#include <httpd.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_request.h>
#include <apr_strings.h>

#ifdef __cplusplus
  #define EXTERN_C extern "C"
#else 
  #define EXTERN_C
#endif // EXTERN_C

/**********************
 * APACHE API SECTION *
 **********************/

/**
 * @brief Register all hooks needed for grader module.
 * 
 * @param pool Apache internal pool structure.
 * @return void
 */
EXTERN_C void register_hooks(apr_pool_t* pool);

/**
 * @brief Handles requests to grader, these are requests with '.grade' extension. 
 * @details There are two major code paths depending on the type of request. 
 * 
 * 1) In case of POST request we are expecting file to be given (the file that needs to be graded) and a few
 * test cases. File will be read into the memory and written to disk, but test cases will reside in memory for now. 
 * After that grader::task object will be created in interprocess shared memory and its id will be used as a handle to task
 * (managed_shared_memory segment(open_only, "MySharedMemory");, res = segment.find<grader::task>("some_nice_uuid");).
 * Also, its id will be appended to task_queue so task can be fetched from grader process using this id. Finally this id
 * will be returned to HTTP client issuing original request so grader::task status can be queried using this id.
 * 
 * 2) In case of GET request we are expecting to get query string like "?id='some_nice_uuid'" where uuid is uuid returned
 * to client when file was submitted. Task whose id is given uuid will be fetched from interprocess shared memory using this uuid.
 * Client will receive json encoded data in one of three forms:
 *   a) { "ERROR": "Some very useful error message client will be pleased with." }
 *   b) { "STATE": "WHICH_STATE" }, where WHICH_STATE can be: WAITING, COMPILING, RUNNING
 *   c) { 
 *         "SUCCESS": "" 
 *         "TEST0": "FAILED" # This means test case failed
 *         "TEST1": ""       # This means test case passed
 *         .
 *         .
 *         .
 *      }, all tests will be listed in order and will contain FAILED or empty string
 * 
 * @param r Incoming request.
 * @return int HTTP status code.
 */
EXTERN_C int grader_handler(request_rec* r);

module AP_MODULE_DECLARE_DATA grader_module = 
{
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  register_hooks
};

/***************************
 * C++ DEFINITIONS SECTION *
 ***************************/

namespace grader
{
  struct limits
  {
    using size_type = std::size_t;
    
    static constexpr size_type MAX_BODY_LENGTH = (1U << 20) * 20; // 20MB
  };
  
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
  int read_body(request_rec *r, const char **rbuf, apr_off_t *size);

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
  char* boundary(request_rec* r);
}

#endif // MOD_GRADER_H