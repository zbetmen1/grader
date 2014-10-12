#ifndef REFLECTION_TYPES
#define REFLECTION_TYPES

#include <string>

namespace reflection 
{
  class object;
  
  typedef void (*object_dtor)(void*);
  typedef object* (*object_ctor)(void);
  
  using shared_lib_impl = void*;
  using path = std::string;
}

#endif // REFLECTION_TYPES