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
EXTERN_C int grader_handler(request_rec* r);
EXTERN_C char* task_id_from_url(request_rec* r);

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

void avoid_zombie_handler(int);

#endif // MOD_GRADER_H