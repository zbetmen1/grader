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

#endif // MOD_GRADER_H